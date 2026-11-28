// Backend for Teensy 4.0, 4.1 and MicroMod (i.MX RT1062): one channel of a QuadTimer (TMR) per channel of the
// library, one event per interrupt.
//
// How the events are timed. The timer channel counts the bus clock (150 MHz) and never stops; its 16-bit
// counter runs around every 437 us. Every step has a due time, a value of that counter, and the due time of
// the next step is the due time of this one plus the delay of the next event. The compare register is set
// to the due time, and the compare interrupt makes the step:
//
//     due time of a step:            set the step pins, tell the library if a target was reached, fetch the
//                                    next event, set the compare register to the end of the pulse
//     due time + pulse width:        step pins back to idle, DIR pins for the next event, compare register
//                                    to the due time of the next step
//
// Because every due time is counted from the previous due time and not from the moment the interrupt was
// served, a late interrupt delays one edge but nothing after it: the steps do not drift. If an interrupt comes
// so late that the next due time has passed already, the next action is made at once.
//
// The width of the pulse. An interrupt that is served late makes its step late. The end of the pulse is
// counted from the moment the step pins were really set, so that the pulse keeps its width; the due time of
// the next step stays where it was.
//
// The setup time of DIR. Where the DIR pin of a motor changes, its step is made no earlier than the setup
// time after the pin was written: the due time of the step is moved back if it is closer, and with it all
// steps that follow.
//
// maxDelay() tells the library the longest delay the counter can time; the library hands out longer waits in
// pieces.

#if defined(__IMXRT1062__)

#include "Teensy4Backend.h"
#include "../Backend.h"
#include "Arduino.h"

extern "C" void unused_interrupt_vector(void); // what the core puts into every interrupt vector that is not used

namespace TeensyStep
{
    namespace backend
    {
        namespace
        {
            constexpr uint16_t countsPerEvent = 60000; // longest delay of one event, in timer counts (the counter has 16 bit)
            constexpr uint16_t minCounts      = 120;   // shortest time between the end of a pulse and the next step (0.8 us at 150 MHz)

            // Priority of the timer interrupts, see defaultInterruptPriority and setInterruptPriority().
            uint8_t interruptPriority = defaultInterruptPriority;

            // The 16 timer channels, in the order in which they are taken:
            //   1. channels that cannot make PWM on a pin of the board;
            //   2. channels whose PWM pin belongs to a motor of the move (nothing else can need that timer);
            //   3. the other channels, as long as their pin is not switched to PWM.
            // A channel on which another library has enabled an interrupt is never taken.
            struct Timer
            {
                uint8_t module; // 0..3: TMR1..TMR4
                uint8_t channel;
                uint8_t pwmPin; // pin on which this channel makes PWM with analogWrite(), noPin: none
            };
            const Timer timers[] = {
                {3, 0, noPin}, {3, 1, noPin}, {3, 2, noPin}, {3, 3, noPin}, {0, 3, noPin}, {1, 1, noPin}, {1, 2, noPin}, {1, 3, noPin},
                {0, 0, 10}, {0, 2, 11}, {0, 1, 12}, {1, 0, 13}, {2, 2, 14}, {2, 3, 15}, {2, 1, 18}, {2, 0, 19},
            };
            constexpr unsigned timerCount = sizeof(timers) / sizeof(timers[0]);

            IMXRT_TMR_t* const modules[4]   = {&IMXRT_TMR1, &IMXRT_TMR2, &IMXRT_TMR3, &IMXRT_TMR4};
            const IRQ_NUMBER_t moduleIrq[4] = {IRQ_QTIMER1, IRQ_QTIMER2, IRQ_QTIMER3, IRQ_QTIMER4};

            struct Motor // the pins of a motor as registers, for the interrupt
            {
                volatile uint32_t* stepActive; // register that sets the step pin to its active level
                volatile uint32_t* stepIdle;
                uint32_t stepBit;
                volatile uint32_t* dirUp; // register that sets DIR to "count up"; nullptr: no DIR pin
                volatile uint32_t* dirDown;
                uint32_t dirBit;
            };

            enum class Phase : uint8_t {
                closed,
                beforeStep, // waiting for the due time of the current event
                inPulse,    // the steps of the current event are made, their pulse is running
            };

            struct Registers // a timer channel as the library found it; put back when the channel is released
            {
                uint16_t ctrl, sctrl, csctrl, load, comp1, comp2, cmpld1, cmpld2, filt, dma;
                bool enabled; // its bit in the enable register of the module
            };

            struct Chan
            {
                Registers found      = {};
                Phase phase          = Phase::closed;
                IMXRT_TMR_CH_t* tmr  = nullptr;
                int8_t timer         = -1; // index into timers[]
                uint8_t count        = 0;
                uint16_t pulseCounts = 750;
                float countsPerSecond = 150e6f;
                float rest            = 0;     // fraction of a count left over from the last conversion
                uint16_t due          = 0;     // counter value at which the current event is due
                uint16_t nextTotal    = 0;     // counts from the current event to the next one
                bool started          = false;
                bool hasNext          = false; // `next` holds a fetched event
                bool last             = false; // fetch() has returned false: the current event is the last one
                uint16_t dirs         = 0;     // bit i: DIR of motor i is at "count up"
                uint16_t dirsKnown    = 0;     // bit i: `dirs` holds the level of the DIR pin of motor i
                uint16_t dirCounts    = 750;   // setup time of DIR in timer counts
                uint16_t late         = 0;     // counts by which the steps of the current event were made after their due time
                Event cur, next;
                Motor motors[maxMotors];
            };

            Chan chans[maxChannels];
            int8_t ownerOfTimer[timerCount]; // channel of the library that uses the timer, -1: none
            int8_t channelOfModule[4][4];    // the same by module and timer channel, for the interrupts
            bool tablesReady = false;
            uint32_t lockDepth = 0;
            uint32_t savedPrimask = 0;
            volatile uint32_t callsWithoutWork = 0;

            void isrModule0();
            void isrModule1();
            void isrModule2();
            void isrModule3();
            void (*const moduleIsr[4])() = {isrModule0, isrModule1, isrModule2, isrModule3};

            void prepareTables()
            {
                if (tablesReady) return;
                for (int8_t& o : ownerOfTimer) o = -1;
                for (auto& m : channelOfModule)
                    for (int8_t& o : m) o = -1;
                tablesReady = true;
            }

            // Delay of an event in timer counts. The fraction is carried over, so that the steps do not drift. A
            // step needs at least the time of the pulse before it and a short pause.
            inline uint16_t toCounts(Chan& c, const Event& e, bool afterPulse)
            {
                float counts = e.delay * c.countsPerSecond + c.rest;
                uint32_t n   = (uint32_t)counts;
                c.rest       = counts - (float)n;
                uint32_t least = afterPulse ? (uint32_t)c.pulseCounts + minCounts : minCounts;
                if (n < least) n = least;
                if (n > 65000) n = 65000;
                return (uint16_t)n;
            }

            // Sets the DIR pins of the motors that step in the event. Only pins that have to change are written.
            // @return true if a pin was written: the step has to wait for the setup time
            inline bool setDirs(Chan& c, const Event& e)
            {
                uint32_t change = (uint32_t)(((e.dirMask ^ c.dirs) | ~c.dirsKnown) & e.stepMask);
                if (change == 0) return false;
                c.dirs       = (uint16_t)((c.dirs & ~change) | (e.dirMask & change));
                c.dirsKnown  = (uint16_t)(c.dirsKnown | change);
                bool written = false;
                for (uint32_t m = change; m != 0; m &= m - 1)
                {
                    unsigned i     = (unsigned)__builtin_ctz(m);
                    const Motor& k = c.motors[i];
                    if (!k.dirUp) continue;
                    if ((e.dirMask >> i) & 1)
                        *k.dirUp = k.dirBit;
                    else
                        *k.dirDown = k.dirBit;
                    written = true;
                }
                return written;
            }

            // The enable register is shared by the four channels of a module.
            void enableTimer(const Timer& timer, bool on)
            {
                lock();
                if (on)
                    modules[timer.module]->ENBL |= (uint16_t)(1u << timer.channel);
                else
                    modules[timer.module]->ENBL &= (uint16_t) ~(1u << timer.channel);
                unlock();
            }

            void releaseTimer(unsigned ch)
            {
                Chan& c = chans[ch];
                if (c.tmr) // stop, and put the channel back as it was found (the core sets the QuadTimers up for PWM at start)
                {
                    c.tmr->CSCTRL = 0;
                    c.tmr->CTRL   = 0;
                    c.tmr->CNTR   = 0;
                    c.tmr->LOAD   = c.found.load;
                    c.tmr->COMP1  = c.found.comp1;
                    c.tmr->COMP2  = c.found.comp2;
                    c.tmr->CMPLD1 = c.found.cmpld1;
                    c.tmr->CMPLD2 = c.found.cmpld2;
                    c.tmr->FILT   = c.found.filt;
                    c.tmr->DMA    = c.found.dma;
                    c.tmr->SCTRL  = c.found.sctrl;
                    c.tmr->CSCTRL = c.found.csctrl;
                    if (!c.found.enabled) enableTimer(timers[c.timer], false);
                    c.tmr->CTRL = c.found.ctrl;
                }
                if (c.timer >= 0)
                {
                    ownerOfTimer[c.timer] = -1;
                    channelOfModule[timers[c.timer].module][timers[c.timer].channel] = -1;
                }
                c.tmr   = nullptr;
                c.timer = -1;
                c.phase = Phase::closed;
            }

            // The edge that is due on the step pins of a channel: the steps of the current event, or the end of
            // their pulse.
            inline void makeEdge(const Chan& c)
            {
                bool step = c.phase == Phase::beforeStep;
                for (uint32_t m = c.cur.stepMask; m != 0; m &= m - 1)
                {
                    const Motor& k = c.motors[__builtin_ctz(m)];
                    if (step)
                        *k.stepActive = k.stepBit;
                    else
                        *k.stepIdle = k.stepBit;
                }
            }

            // One compare interrupt of a channel. `edgeDone`: the edge that is due was made already.
            inline void service(unsigned ch, bool edgeDone)
            {
                Chan& c             = chans[ch];
                IMXRT_TMR_CH_t* tmr = c.tmr;
                for (;;)
                {
                    if (c.phase == Phase::beforeStep) // the current event is due: make its steps
                    {
                        tmr->CSCTRL = TMR_CSCTRL_TCF1EN; // clears the compare flag; one write instead of read and write
                        bool pulse = c.cur.stepMask != 0; // false: a wait, nothing to end
                        if (!edgeDone)
                        {
                            makeEdge(c);
                            c.late = pulse ? (uint16_t)(tmr->CNTR - c.due) : 0;
                        }
                        edgeDone       = false;
                        c.phase        = Phase::inPulse;
                        uint16_t width = (uint16_t)(c.late + c.pulseCounts); // of the pulse, counted from the due time
                        if (pulse) tmr->COMP1 = (uint16_t)(c.due + width);   // the end of the pulse

                        if (c.cur.arrived)
                        {
                            arrived(ch); // the sketch may give a command, also an emergency stop
                            if (c.phase != Phase::inPulse) return;
                        }
                        c.hasNext = fetch(ch, c.next);
                        c.last    = !c.hasNext;
                        if (c.hasNext) c.nextTotal = toCounts(c, c.next, c.cur.stepMask != 0);

                        if (pulse)
                        {
                            if ((uint16_t)(tmr->CNTR - c.due) < width) return; // wait for the end of the pulse
                            tmr->CSCTRL = TMR_CSCTRL_TCF1EN;                           // it has passed already
                        }
                    }
                    else
                        tmr->CSCTRL = TMR_CSCTRL_TCF1EN;

                    // the end of the pulse (of a wait: nothing)
                    if (!edgeDone) makeEdge(c);
                    edgeDone = false;
                    if (c.last)
                    {
                        releaseTimer(ch);
                        finished(ch);
                        return;
                    }
                    uint16_t previous = c.due;
                    if (c.late != 0) // the pulse ended later: the pause after it is kept
                    {
                        uint32_t least = (uint32_t)c.late + c.pulseCounts + minCounts;
                        if (c.cur.stepMask != 0 && least > c.nextTotal) c.nextTotal = (uint16_t)(least > 65000 ? 65000 : least);
                        c.late = 0;
                    }
                    c.due             = previous + c.nextTotal;
                    c.cur             = c.next;
                    c.hasNext         = false;
                    c.phase           = Phase::beforeStep;
                    if (setDirs(c, c.cur)) // the step no earlier than the setup time from now
                    {
                        uint32_t least = (uint32_t)(uint16_t)(tmr->CNTR - previous) + c.dirCounts;
                        if (least > 65000) least = 65000;
                        if (least > c.nextTotal)
                        {
                            c.nextTotal = (uint16_t)least;
                            c.due       = (uint16_t)(previous + c.nextTotal);
                        }
                    }
                    tmr->COMP1 = c.due;
                    if ((uint16_t)(tmr->CNTR - previous) < c.nextTotal) return; // the next event is still ahead
                    // it is due already (the interrupt was served late): make it now
                }
            }

            // The interrupt of a QuadTimer module, shared by its four channels. Accesses to the timer registers
            // are slow (they cross from the processor bus to the peripheral bus), so only the channels that the
            // library uses are looked at.
            //
            // When several channels are due at the same time, first the edges of all of them are made and only
            // then the rest of the work (fetching the next events) is done. Otherwise the edge of one motor
            // would wait for the calculations of another one.
            inline void serviceModule(unsigned module)
            {
                uint8_t due[4];
                unsigned n = 0;
                for (unsigned k = 0; k < 4; k++)
                {
                    int8_t ch = channelOfModule[module][k];
                    if (ch < 0 || !(modules[module]->CH[k].CSCTRL & TMR_CSCTRL_TCF1)) continue;
                    Chan& c = chans[ch];
                    makeEdge(c);
                    if (c.phase == Phase::beforeStep) c.late = c.cur.stepMask != 0 ? (uint16_t)(modules[module]->CH[k].CNTR - c.due) : 0;
                    due[n++] = (uint8_t)ch;
                }
                for (unsigned i = 0; i < n; i++) service(due[i], true);

                // When an event became due while this call was running, service() has made it at once. The
                // processor has noted the request of that event nevertheless and would call the interrupt
                // again, for nothing. The request is taken back here; it stays if a channel still has
                // something due (its flag holds the interrupt line active).
                // No `dsb` is needed before the return: service() clears the flag first and reads or writes
                // the timer again after its work. A call that finds no flag is counted (see Teensy4Backend.h).
                NVIC_CLEAR_PENDING(moduleIrq[module]);
                if (n == 0) callsWithoutWork = callsWithoutWork + 1;
            }

            void isrModule0() { serviceModule(0); }
            void isrModule1() { serviceModule(1); }
            void isrModule2() { serviceModule(2); }
            void isrModule3() { serviceModule(3); }

            // Is the timer channel in use by something else than this library?
            bool inUse(const Timer& timer)
            {
                IMXRT_TMR_CH_t* tmr = &modules[timer.module]->CH[timer.channel];
                if ((tmr->CSCTRL & (TMR_CSCTRL_TCF1EN | TMR_CSCTRL_TCF2EN)) || (tmr->SCTRL & TMR_SCTRL_TCFIE)) return true; // someone uses its interrupt
                if (timer.pwmPin != noPin) return (*portConfigRegister(timer.pwmPin) & 7) == 1;                             // its pin is switched to the timer: PWM is on
                uint16_t ctrl = tmr->CTRL;
                return ctrl != 0 && ctrl != 0x3026; // neither stopped nor as the core leaves it at start
            }

            // Can the timer be taken for this move, in round 1, 2 or 3 of the order above?
            bool usable(unsigned t, const ChannelSetup& setup, int round)
            {
                if (ownerOfTimer[t] >= 0) return false;
                const Timer& timer = timers[t];

                // the interrupt of the module belongs to another library
                void (*vector)() = _VectorsRam[moduleIrq[timer.module] + 16];
                if (vector != moduleIsr[timer.module] && vector != &unused_interrupt_vector) return false;

                bool own = false;
                for (unsigned i = 0; i < setup.count; i++) own = own || setup.motors[i].step == timer.pwmPin || setup.motors[i].dir == timer.pwmPin;
                if (round == 1) return timer.pwmPin == noPin && !inUse(timer);
                if (round == 2) return timer.pwmPin != noPin && own;
                return timer.pwmPin != noPin && !inUse(timer);
            }
        }

        //----------------------------------------------------------------------------------------------------
        // the backend interface

        float maxDelay()
        {
            return (float)countsPerEvent / (float)F_BUS_ACTUAL;
        }

        void initMotor(const MotorPins& pins)
        {
            if (pins.step != noPin)
            {
                digitalWriteFast(pins.step, pins.stepActiveHigh ? LOW : HIGH);
                pinMode(pins.step, OUTPUT);
            }
            if (pins.dir != noPin)
            {
                digitalWriteFast(pins.dir, pins.dirHighCountsUp ? HIGH : LOW);
                pinMode(pins.dir, OUTPUT);
            }
        }

        void setPin(uint8_t pin, bool level)
        {
            if (pin == noPin) return;
            digitalWriteFast(pin, level ? HIGH : LOW);
            pinMode(pin, OUTPUT);
        }

        errorCode open(unsigned channel, const ChannelSetup& setup)
        {
            prepareTables();
            int found = -1;
            for (int round = 1; round <= 3 && found < 0; round++)
                for (unsigned t = 0; t < timerCount && found < 0; t++)
                    if (usable(t, setup, round)) found = (int)t;
            if (found < 0) return errorCode::noTimer;

            Chan& c            = chans[channel];
            c                  = Chan();
            c.timer            = (int8_t)found;
            c.count            = (uint8_t)setup.count;
            c.countsPerSecond  = (float)F_BUS_ACTUAL;
            float pulse        = setup.pulseWidth * 1e-6f * c.countsPerSecond;
            c.pulseCounts      = pulse < minCounts ? minCounts : pulse > 30000 ? 30000 : (uint16_t)pulse;
            float dirSetup     = setup.dirSetup * 1e-6f * c.countsPerSecond;
            c.dirCounts        = dirSetup > 30000 ? 30000 : dirSetup > 0 ? (uint16_t)(dirSetup + 1) : 0;
            for (unsigned i = 0; i < setup.count; i++)
            {
                const MotorPins& p = setup.motors[i];
                Motor& k           = c.motors[i];
                k.stepBit          = digitalPinToBitMask(p.step);
                k.stepActive       = p.stepActiveHigh ? portSetRegister(p.step) : portClearRegister(p.step);
                k.stepIdle         = p.stepActiveHigh ? portClearRegister(p.step) : portSetRegister(p.step);
                if (p.dir == noPin)
                {
                    k.dirUp = k.dirDown = nullptr;
                    k.dirBit            = 0;
                    continue;
                }
                k.dirBit  = digitalPinToBitMask(p.dir);
                k.dirUp   = p.dirHighCountsUp ? portSetRegister(p.dir) : portClearRegister(p.dir);
                k.dirDown = p.dirHighCountsUp ? portClearRegister(p.dir) : portSetRegister(p.dir);
            }

            const Timer& timer = timers[found];
            switch (timer.module)
            {
                case 0: CCM_CCGR6 |= CCM_CCGR6_QTIMER1(CCM_CCGR_ON); break;
                case 1: CCM_CCGR6 |= CCM_CCGR6_QTIMER2(CCM_CCGR_ON); break;
                case 2: CCM_CCGR6 |= CCM_CCGR6_QTIMER3(CCM_CCGR_ON); break;
                default: CCM_CCGR6 |= CCM_CCGR6_QTIMER4(CCM_CCGR_ON); break;
            }
            c.tmr         = &modules[timer.module]->CH[timer.channel];
            c.found       = {c.tmr->CTRL, c.tmr->SCTRL, c.tmr->CSCTRL, c.tmr->LOAD, c.tmr->COMP1, c.tmr->COMP2, c.tmr->CMPLD1, c.tmr->CMPLD2,
                             c.tmr->FILT, c.tmr->DMA, ((modules[timer.module]->ENBL >> timer.channel) & 1) != 0};

            // Every register of the channel gets a known value; nothing is left as another program part (or
            // the core, which sets the QuadTimers up for PWM at start) has set it.
            c.tmr->CTRL   = 0; // stopped
            c.tmr->CSCTRL = 0; // no interrupt, no reload of the compare registers, flags cleared
            c.tmr->SCTRL  = 0; // no interrupt, no capture, output off, flags cleared
            c.tmr->FILT   = 0;
            c.tmr->DMA    = 0;
            c.tmr->CNTR   = 0;
            c.tmr->LOAD   = 0;
            c.tmr->COMP1  = 0;
            c.tmr->COMP2  = 0;
            c.tmr->CMPLD1 = 0;
            c.tmr->CMPLD2 = 0;
            enableTimer(timer, true);
            ownerOfTimer[found] = (int8_t)channel;
            channelOfModule[timer.module][timer.channel] = (int8_t)channel;

            attachInterruptVector(moduleIrq[timer.module], moduleIsr[timer.module]);
            NVIC_SET_PRIORITY(moduleIrq[timer.module], interruptPriority);
            NVIC_ENABLE_IRQ(moduleIrq[timer.module]);
            c.phase = Phase::beforeStep;
            return errorCode::OK;
        }

        void start(unsigned channel)
        {
            Chan& c = chans[channel];
            if (c.phase == Phase::closed) return;
            if (c.started)
            {
                if (c.last) // the last event is being executed and a new command has come: go on
                {
                    c.hasNext = fetch(channel, c.next);
                    c.last    = !c.hasNext;
                    if (c.hasNext) c.nextTotal = toCounts(c, c.next, c.cur.stepMask != 0);
                }
                return;
            }
            if (!fetch(channel, c.cur)) // nothing to do at all
            {
                releaseTimer(channel);
                finished(channel);
                return;
            }
            c.started = true;
            c.last    = false;
            c.hasNext = false;
            c.phase   = Phase::beforeStep;
            bool dirWritten = setDirs(c, c.cur);

            c.due = toCounts(c, c.cur, false);
            if (dirWritten && c.due < c.dirCounts) c.due = c.dirCounts;
            c.tmr->CNTR   = 0;
            c.tmr->LOAD   = 0;
            c.tmr->COMP1  = c.due;
            c.tmr->CSCTRL = TMR_CSCTRL_TCF1EN;
            c.tmr->CTRL   = TMR_CTRL_CM(1) | TMR_CTRL_PCS(8); // count the bus clock, around and around
        }

        void halt(unsigned channel, int32_t* notExecuted)
        {
            Chan& c = chans[channel];
            for (unsigned i = 0; i < maxMotors; i++) notExecuted[i] = 0;
            if (c.phase == Phase::closed) return;

            if (c.tmr) c.tmr->CSCTRL &= ~TMR_CSCTRL_TCF1EN; // no further interrupt of this channel
            auto count = [&](const Event& e) {
                for (uint32_t m = e.stepMask; m != 0; m &= m - 1)
                {
                    unsigned i = (unsigned)__builtin_ctz(m);
                    notExecuted[i] += ((e.dirMask >> i) & 1) ? 1 : -1;
                }
            };
            if (c.started && c.phase == Phase::beforeStep) count(c.cur); // fetched, its steps not made yet
            if (c.started && c.phase == Phase::inPulse)
            {
                if (c.hasNext) count(c.next);
                delayNanoseconds((uint32_t)((float)c.pulseCounts * 1e9f / c.countsPerSecond)); // let the pulse that has begun end
                for (uint32_t m = c.cur.stepMask; m != 0; m &= m - 1)
                {
                    const Motor& k = c.motors[__builtin_ctz(m)];
                    *k.stepIdle    = k.stepBit;
                }
            }
            releaseTimer(channel);
        }

        void lock()
        {
            uint32_t primask;
            __asm__ volatile("mrs %0, primask" : "=r"(primask));
            __disable_irq();
            if (lockDepth++ == 0) savedPrimask = primask;
        }

        void unlock()
        {
            if (--lockDepth == 0 && (savedPrimask & 1) == 0) __enable_irq();
        }

        void idle()
        {
            yield();
        }

        void setInterruptPriority(uint8_t priority)
        {
            interruptPriority = priority;
            for (unsigned m = 0; m < 4; m++) // the timers the library uses now; the others get it when they are taken
                if (_VectorsRam[moduleIrq[m] + 16] == moduleIsr[m]) NVIC_SET_PRIORITY(moduleIrq[m], priority);
        }

        uint32_t teensy4::interruptsWithoutWork()
        {
            return callsWithoutWork;
        }
    }
}

#endif
