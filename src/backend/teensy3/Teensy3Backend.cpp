// Backend for Teensy 3.5 and 3.6 (Kinetis K64, K66): one channel of a FlexTimer (FTM) per channel of the
// library, one event per interrupt.
//
// The timers. The four FlexTimer modules have 20 channels. All channels of a module share one counter, and
// the core has set that counter up for PWM (analogWrite) at start: it counts from 0 to its modulo value and
// starts again, about 488 times per second. The library does not touch the counter. It only switches a
// channel that is not needed for PWM to "compare without a pin" and uses its compare interrupt. PWM on the
// other channels of the module goes on undisturbed, and the interval timers stay free for the sketch.
//
// How the events are timed. Every step has a due time, a value of the counter, and the due time of the next
// step is the due time of this one plus the delay of the next event (modulo the period of the counter). The
// compare register of the channel is set to the due time, and the compare interrupt makes the step:
//
//     due time of a step:            set the step pins, tell the library if a target was reached, fetch the
//                                    next event, set the compare register to the end of the pulse
//     due time + pulse width:        step pins back to idle, DIR pins for the next event, compare register
//                                    to the due time of the next step
//
// Because every due time is counted from the previous due time and not from the moment the interrupt was
// served, a late interrupt delays one edge but nothing after it: the steps do not drift. If an interrupt comes
// so late that the next due time has passed already (or is only a few counts away), the next action is made
// in the same call.
//
// The setup time of DIR. Where the DIR pin of a motor changes, its step is made no earlier than the setup
// time after the pin was written: the due time of the step is moved back if it is closer, and with it all
// steps that follow.
//
// maxDelay() tells the library the longest delay the counter can time; the library hands out longer waits in
// pieces. A module whose counter runs around faster than that (a high PWM frequency was set for its pins)
// is not used.

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)

#include "Teensy3Backend.h"
#include "../Backend.h"
#include "../../core/Hot.h"
#include "Arduino.h"
#include <cstring>

extern "C" void unused_isr(void); // what the core puts into every interrupt vector that is not used

namespace TeensyStep
{
    namespace backend
    {
        namespace
        {
            constexpr float longestDelay   = 0.001f;  // seconds; longest delay of one event
            constexpr float shortestPeriod = 0.0012f; // seconds; a counter that runs around faster is not used
            constexpr float pauseTime      = 0.8e-6f; // seconds; shortest time between the end of a pulse and the next step
            constexpr int32_t guard        = 4;       // counts; a compare value this close ahead of the counter may be missed
            constexpr float shortPulse     = 2.0e-6f; // seconds; a pulse up to this width is ended in the interrupt that started it

            // Priority of the timer interrupts, see defaultInterruptPriority and setInterruptPriority().
            uint8_t interruptPriority = defaultInterruptPriority;

            struct Ftm // the registers of a FlexTimer module that are used here
            {
                volatile uint32_t SC, CNT, MOD;
                struct
                {
                    volatile uint32_t SC, V;
                } CH[8];
                volatile uint32_t CNTIN, STATUS;
            };
            constexpr uint32_t offsetQuadCtrl = 0x80; // quadrature decoder control register

            constexpr uint32_t compareMode = FTM_CSC_MSA;                // compare, pin not used
            constexpr uint32_t compareOn   = FTM_CSC_MSA | FTM_CSC_CHIE; // with its interrupt
            constexpr uint32_t modeBits    = 0x3C;                       // MSB, MSA, ELSB, ELSA
            constexpr uint32_t pwmMode     = 0x28;                       // as the core sets every channel at start

            Ftm* const modules[4]           = {(Ftm*)&FTM0_SC, (Ftm*)&FTM1_SC, (Ftm*)&FTM2_SC, (Ftm*)&FTM3_SC};
            const IRQ_NUMBER_t moduleIrq[4] = {IRQ_FTM0, IRQ_FTM1, IRQ_FTM2, IRQ_FTM3};

            // The 20 timer channels, in the order in which they are taken:
            //   1. channels whose PWM pin belongs to a motor of the move (nothing else can need that channel);
            //   2. the other channels, as long as their pin is not switched to PWM.
            // A channel on which something else has enabled an interrupt or another mode is never taken, and
            // no channel of a module whose interrupt belongs to another library.
            struct Timer
            {
                uint8_t module; // 0..3: FTM0..FTM3
                uint8_t channel;
                uint8_t pwmPin; // pin on which this channel makes PWM with analogWrite()
                uint8_t mux;    // function number of that pin for the timer
            };
            const Timer timers[] = {
                {3, 0, 2, 4}, {3, 1, 14, 4}, {3, 2, 7, 4}, {3, 3, 8, 4}, {3, 4, 35, 3}, {3, 5, 36, 3}, {3, 6, 37, 3}, {3, 7, 38, 3},
                {0, 0, 22, 4}, {0, 1, 23, 4}, {0, 2, 9, 4}, {0, 3, 10, 4}, {0, 4, 6, 4}, {0, 5, 20, 4}, {0, 6, 21, 4}, {0, 7, 5, 4},
                {2, 0, 29, 3}, {2, 1, 30, 3}, {1, 0, 3, 3}, {1, 1, 4, 3},
            };
            constexpr unsigned timerCount = sizeof(timers) / sizeof(timers[0]);

            struct Motor // the pins of a motor as registers, for the interrupt (each register is one pin: bit-band)
            {
                volatile uint8_t* stepActive; // writing 1 sets the step pin to its active level
                volatile uint8_t* stepIdle;
                volatile uint8_t* dirUp; // writing 1 sets DIR to "count up"; nullptr: no DIR pin
                volatile uint8_t* dirDown;
            };

            enum class Phase : uint8_t {
                closed,
                beforeStep, // waiting for the due time of the current event
                inPulse,    // the steps of the current event are made, their pulse is running
            };

            struct Chan
            {
                Phase phase           = Phase::closed;
                Ftm* ftm              = nullptr;
                volatile uint32_t* sc = nullptr; // status and control register of the timer channel
                volatile uint32_t* cv = nullptr; // its compare value
                uint32_t foundSc = 0, foundV = 0; // the timer channel as it was found; put back at the end
                int8_t timer          = -1;      // index into timers[]
                uint8_t count         = 0;
                int32_t period        = 61440;   // counts of one round of the counter
                int32_t maxCounts     = 50000;   // longest delay of one event, in counts
                int32_t minCounts     = 24;      // shortest pause between a pulse and the next step
                int32_t pulseCounts   = 150;
                int32_t dirCounts     = 150;     // setup time of DIR
                float countsPerSecond = 30e6f;
                uint32_t delayBits    = 0xFFFFFFFF; // the delay converted last, as its bit pattern
                uint32_t delayQ16     = 0;          // and its counts, with 16 fractional bits
                uint32_t restQ16      = 0;          // fraction of a count left over from the last event
                bool waitPulse        = false;      // the pulse is short: it is waited for, without a compare event
                bool armed            = false;      // the compare interrupt of the timer channel is on
                uint16_t dirs         = 0;          // bit i: DIR of motor i is at "count up"
                uint16_t dirsKnown    = 0;          // bit i: `dirs` holds the level of the DIR pin of motor i
                int32_t due           = 0;     // counter value at which the current event is due
                int32_t nextTotal     = 0;     // counts from the current event to the next one
                bool started          = false;
                bool hasNext          = false; // `next` holds a fetched event
                bool last             = false; // fetch() has returned false: the current event is the last one
                Event cur, next;
                Motor motors[maxMotors];
            };

            Chan chans[maxChannels];
            int8_t ownerOfTimer[timerCount]; // channel of the library that uses the timer, -1: none
            int8_t channelOfModule[4][8];    // the same by module and timer channel, for the interrupts
            uint8_t ownedOfModule[4];        // bit k: timer channel k of the module is used by the library
            bool tablesReady       = false;
            bool madeLateEvent     = false; // an interrupt has made an event that was due already
            uint32_t lockDepth     = 0;
            uint32_t savedPrimask  = 0;
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
                for (uint8_t& o : ownedOfModule) o = 0;
                tablesReady = true;
            }

            // Counts since the counter had the value `since` (less than one round).
            inline int32_t elapsed(const Chan& c, int32_t since)
            {
                int32_t e = (int32_t)c.ftm->CNT - since;
                return e < 0 ? e + c.period : e;
            }

            inline int32_t wrapped(const Chan& c, int32_t counts) // a counter value from a sum of two
            {
                return counts >= c.period ? counts - c.period : counts;
            }

            // Has the time `counts` after `since` come? Also true a few counts before it: a compare value
            // that close to the counter may not be taken over in time, so the caller waits these counts.
            inline bool reached(const Chan& c, int32_t since, int32_t counts)
            {
                return elapsed(c, since) + guard >= counts;
            }

            inline void waitFor(const Chan& c, int32_t since, int32_t counts)
            {
                while (elapsed(c, since) < counts) {} // at most `guard` counts
            }

            inline void clearFlag(const Chan& c) // read, then write the flag as 0
            {
                (void)*c.sc;
                *c.sc = compareOn;
            }

            // Delay of an event in timer counts. The fraction is carried over, so that the steps do not drift. A
            // step needs at least the time of the pulse before it and a short pause.
            // At constant speed all events have the same delay; it is converted once, and the interrupt then
            // works without floating point (which would cost the processor the saving of its registers).
            inline int32_t toCounts(Chan& c, const Event& e, bool afterPulse)
            {
                uint32_t bits;
                memcpy(&bits, &e.delay, sizeof(bits));
                if (bits != c.delayBits)
                {
                    c.delayBits  = bits;
                    float counts = e.delay * c.countsPerSecond;
                    c.delayQ16   = counts < 65000.0f ? (uint32_t)(counts * 65536.0f) : 65000u << 16;
                }
                uint32_t sum  = c.delayQ16 + c.restQ16;
                c.restQ16     = sum & 0xFFFF;
                int32_t n     = (int32_t)(sum >> 16);
                int32_t least = afterPulse ? c.pulseCounts + c.minCounts : c.minCounts;
                if (n < least) n = least;
                if (n > c.maxCounts) n = c.maxCounts;
                return n;
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
                        *k.dirUp = 1;
                    else
                        *k.dirDown = 1;
                    written = true;
                }
                return written;
            }

            void releaseTimer(unsigned ch)
            {
                Chan& c = chans[ch];
                if (c.sc) // stop, and put the timer channel back as it was found
                {
                    *c.sc = compareMode; // no interrupt; in this mode a new value is taken over with the next count
                    *c.cv = c.foundV;
                    (void)c.ftm->CNT; // two accesses to the timer take longer than one count
                    (void)c.ftm->CNT;
                    *c.sc = c.foundSc & ~(uint32_t)FTM_CSC_CHF;
                }
                if (c.timer >= 0)
                {
                    const Timer& timer = timers[c.timer];
                    ownerOfTimer[c.timer] = -1;
                    channelOfModule[timer.module][timer.channel] = -1;
                    ownedOfModule[timer.module] &= (uint8_t) ~(1u << timer.channel);
                }
                c.armed = false;
                c.sc    = nullptr;
                c.cv    = nullptr;
                c.ftm   = nullptr;
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
                        *k.stepActive = 1;
                    else
                        *k.stepIdle = 1;
                }
            }

            // One compare interrupt of a channel; its flag is cleared already. `edgeDone`: the edge that is due
            // was made already.
            __attribute__((always_inline)) inline void service(unsigned ch, bool edgeDone)
            {
                Chan& c = chans[ch];
                for (;;)
                {
                    if (c.phase == Phase::beforeStep) // the current event is due: make its steps
                    {
                        if (!edgeDone) makeEdge(c);
                        edgeDone   = false;
                        c.phase    = Phase::inPulse;
                        bool pulse = c.cur.stepMask != 0;                                           // false: a wait, nothing to end
                        if (pulse && !c.waitPulse) *c.cv = (uint32_t)wrapped(c, c.due + c.pulseCounts); // the end of the pulse

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
                            if (c.waitPulse)
                            {
                                waitFor(c, c.due, c.pulseCounts); // a short pulse: no second interrupt for its end
                            }
                            else
                            {
                                if (!reached(c, c.due, c.pulseCounts)) return; // wait for the end of the pulse
                                waitFor(c, c.due, c.pulseCounts);              // it has passed already, or nearly
                                clearFlag(c);
                                madeLateEvent = true;
                            }
                        }
                    }

                    // the end of the pulse (of a wait: nothing)
                    if (!edgeDone) makeEdge(c);
                    edgeDone = false;
                    if (c.last)
                    {
                        releaseTimer(ch);
                        finished(ch);
                        return;
                    }
                    int32_t previous = c.due;
                    c.due            = wrapped(c, previous + c.nextTotal);
                    c.cur            = c.next;
                    c.hasNext        = false;
                    c.phase          = Phase::beforeStep;
                    if (setDirs(c, c.cur)) // the step no earlier than the setup time from now
                    {
                        int32_t least = elapsed(c, previous) + c.dirCounts;
                        if (least > c.maxCounts) least = c.maxCounts;
                        if (least > c.nextTotal)
                        {
                            c.nextTotal = least;
                            c.due       = wrapped(c, previous + least);
                        }
                    }
                    *c.cv = (uint32_t)c.due;
                    if (!reached(c, previous, c.nextTotal)) return; // the next event is still ahead
                    waitFor(c, previous, c.nextTotal);              // it is due already (the interrupt was served late): make it now
                    clearFlag(c);
                    madeLateEvent = true;
                }
            }

            // The interrupt of a FlexTimer module, shared by its channels. Only the channels that the library
            // uses are looked at.
            //
            // When several channels are due at the same time, first the edges of all of them are made and only
            // then the rest of the work (fetching the next events) is done. Otherwise the edge of one motor
            // would wait for the calculations of another one.
            __attribute__((always_inline)) inline void serviceModule(unsigned module)
            {
                // The flags of all channels are read and cleared with one access each (a flag is cleared by
                // writing 0 to it after it was read as 1; the other bits are written as 1, which changes nothing).
                uint32_t flags = modules[module]->STATUS & ownedOfModule[module];
                unsigned n     = 0;
                if (flags != 0)
                {
                    modules[module]->STATUS = ~flags;
                    if ((flags & (flags - 1)) == 0) // one channel, the usual case
                    {
                        n = 1;
                        service((unsigned)channelOfModule[module][__builtin_ctz(flags)], false);
                    }
                    else
                    {
                        uint8_t due[8];
                        for (uint32_t m = flags; m != 0; m &= m - 1)
                        {
                            int8_t ch = channelOfModule[module][__builtin_ctz(m)];
                            makeEdge(chans[ch]);
                            due[n++] = (uint8_t)ch;
                        }
                        for (unsigned i = 0; i < n; i++)
                            if (chans[due[i]].armed) service(due[i], true);
                    }
                }

                // When an event became due while this call was running, service() has made it at once. The
                // processor has noted the request of that event nevertheless and would call the interrupt
                // again, for nothing. The request is taken back here; it stays if a channel still has
                // something due (its flag holds the interrupt line active). A call that finds no flag is
                // counted (see Teensy3Backend.h).
                if (madeLateEvent || n != 1)
                {
                    madeLateEvent = false;
                    NVIC_CLEAR_PENDING(moduleIrq[module]);
                    if (n == 0) callsWithoutWork = callsWithoutWork + 1;
                }
            }

            TEENSYSTEP_HOT void isrModule0() { serviceModule(0); }
            TEENSYSTEP_HOT void isrModule1() { serviceModule(1); }
            TEENSYSTEP_HOT void isrModule2() { serviceModule(2); }
            TEENSYSTEP_HOT void isrModule3() { serviceModule(3); }

            // Can the counter of the module time the events? It has to count the bus clock upwards from 0,
            // slowly enough for the longest delay, and its interrupt must not belong to another library.
            bool moduleUsable(unsigned module)
            {
                Ftm* ftm    = modules[module];
                uint32_t sc = ftm->SC;
                if ((sc & FTM_SC_CLKS_MASK) != FTM_SC_CLKS(1) || (sc & FTM_SC_CPWMS) || ftm->CNTIN != 0) return false;
                if (*(volatile uint32_t*)((uintptr_t)ftm + offsetQuadCtrl) & FTM_QDCTRL_QUADEN) return false; // it counts an encoder
                float period = (float)(ftm->MOD + 1) / (float)(F_BUS >> (sc & FTM_SC_PS_MASK));
                if (period < shortestPeriod) return false;

                void (*vector)() = _VectorsRam[moduleIrq[module] + 16];
                return vector == moduleIsr[module] || vector == &unused_isr;
            }

            // Is the timer channel in use by something else than this library?
            bool inUse(const Timer& timer)
            {
                uint32_t sc = modules[timer.module]->CH[timer.channel].SC;
                if (sc & FTM_CSC_CHIE) return true;                                    // someone uses its interrupt
                if ((sc & modeBits) != pwmMode && (sc & modeBits) != 0) return true;   // or another mode than the core has set
                return (*portConfigRegister(timer.pwmPin) & PORT_PCR_MUX_MASK) == PORT_PCR_MUX(timer.mux); // its pin is switched to the timer: PWM is on
            }

            // Can the timer be taken for this move, in round 1 or 2 of the order above?
            bool usable(unsigned t, const ChannelSetup& setup, int round)
            {
                if (ownerOfTimer[t] >= 0) return false;
                const Timer& timer = timers[t];
                if (!moduleUsable(timer.module)) return false;

                bool own = false;
                for (unsigned i = 0; i < setup.count; i++) own = own || setup.motors[i].step == timer.pwmPin || setup.motors[i].dir == timer.pwmPin;
                uint32_t sc = modules[timer.module]->CH[timer.channel].SC;
                if (round == 1) return own && !(sc & FTM_CSC_CHIE);
                return !inUse(timer);
            }
        }

        //----------------------------------------------------------------------------------------------------
        // the backend interface

        float maxDelay()
        {
            return longestDelay;
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
            for (int round = 1; round <= 2 && found < 0; round++)
                for (unsigned t = 0; t < timerCount && found < 0; t++)
                    if (usable(t, setup, round)) found = (int)t;
            if (found < 0) return errorCode::noTimer;

            const Timer& timer = timers[found];
            Chan& c            = chans[channel];
            c                  = Chan();
            c.timer            = (int8_t)found;
            c.count            = (uint8_t)setup.count;
            c.ftm              = modules[timer.module];
            c.sc               = &c.ftm->CH[timer.channel].SC;
            c.cv               = &c.ftm->CH[timer.channel].V;
            c.period           = (int32_t)(c.ftm->MOD + 1);
            c.countsPerSecond  = (float)(F_BUS >> (c.ftm->SC & FTM_SC_PS_MASK));
            c.maxCounts        = c.period - c.period / 8;
            c.minCounts        = (int32_t)(pauseTime * c.countsPerSecond) + guard;
            float pulse        = setup.pulseWidth * 1e-6f * c.countsPerSecond;
            float longest      = (float)(c.period / 4);
            c.pulseCounts      = pulse < (float)c.minCounts ? c.minCounts : pulse > longest ? (int32_t)longest : (int32_t)pulse;
            c.waitPulse        = setup.pulseWidth * 1e-6f <= shortPulse;
            float dirSetup     = setup.dirSetup * 1e-6f * c.countsPerSecond;
            c.dirCounts        = dirSetup > longest ? (int32_t)longest : dirSetup > 0 ? (int32_t)dirSetup + 1 : 0;
            for (unsigned i = 0; i < setup.count; i++)
            {
                const MotorPins& p = setup.motors[i];
                Motor& k           = c.motors[i];
                k.stepActive       = p.stepActiveHigh ? portSetRegister(p.step) : portClearRegister(p.step);
                k.stepIdle         = p.stepActiveHigh ? portClearRegister(p.step) : portSetRegister(p.step);
                if (p.dir == noPin)
                {
                    k.dirUp = k.dirDown = nullptr;
                    continue;
                }
                k.dirUp   = p.dirHighCountsUp ? portSetRegister(p.dir) : portClearRegister(p.dir);
                k.dirDown = p.dirHighCountsUp ? portClearRegister(p.dir) : portSetRegister(p.dir);
            }

            // The timer channel compares without a pin; its interrupt is switched on at the start.
            c.foundSc = *c.sc;
            c.foundV  = *c.cv;
            *c.sc     = 0;
            *c.sc     = compareMode;

            ownerOfTimer[found] = (int8_t)channel;
            channelOfModule[timer.module][timer.channel] = (int8_t)channel;

            _VectorsRam[moduleIrq[timer.module] + 16] = moduleIsr[timer.module];
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

            int32_t first = toCounts(c, c.cur, false);
            if (first < 8 * guard) first = 8 * guard; // far enough ahead for the compare value to be taken over
            if (dirWritten && first < c.dirCounts) first = c.dirCounts;
            c.due = wrapped(c, (int32_t)c.ftm->CNT + first);
            *c.cv = (uint32_t)c.due;
            lock();
            c.armed = true;
            ownedOfModule[timers[c.timer].module] |= (uint8_t)(1u << timers[c.timer].channel);
            (void)*c.sc;
            *c.sc = compareOn; // flag cleared, interrupt on
            unlock();
        }

        void halt(unsigned channel, int32_t* notExecuted)
        {
            Chan& c = chans[channel];
            for (unsigned i = 0; i < maxMotors; i++) notExecuted[i] = 0;
            if (c.phase == Phase::closed) return;

            if (c.sc) *c.sc = compareMode; // no further interrupt of this channel
            if (c.timer >= 0) ownedOfModule[timers[c.timer].module] &= (uint8_t) ~(1u << timers[c.timer].channel);
            c.armed = false;
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
                delayMicroseconds((uint32_t)((float)c.pulseCounts * 1e6f / c.countsPerSecond) + 1); // let the pulse that has begun end
                for (uint32_t m = c.cur.stepMask; m != 0; m &= m - 1) *c.motors[__builtin_ctz(m)].stepIdle = 1;
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

        uint32_t teensy3::interruptsWithoutWork()
        {
            return callsWithoutWork;
        }
    }
}

#endif
