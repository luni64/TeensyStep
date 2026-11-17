#if !defined(ARDUINO)

#include "HostBackend.h"
#include <deque>
#include <limits>

namespace TeensyStep
{
    namespace backend
    {
        namespace
        {
            struct Queued
            {
                double t; // time at which the event is due
                Event ev;
            };

            struct Chan
            {
                bool open         = false; // holds a timer
                bool running      = false; // start() was called
                bool fetchDone    = false; // fetch() has returned false
                bool startPending = false; // start() was called while the lock was held
                std::vector<MotorPins> motors;
                float pulseWidth = 5, dirSetup = 5;
                std::deque<Queued> queue; // fetched, not yet executed
                double lastQueued = 0;    // due time of the last fetched event
            };

            struct PinChange // a pin change in the future: the end of a step pulse
            {
                double t;
                uint8_t pin;
                bool level;
            };

            Chan chans[maxChannels];
            std::vector<host::Edge> recorded;
            std::vector<PinChange> pinChanges;
            bool levels[256];
            double clock          = 0;
            double lookahead      = 0;
            unsigned timers       = maxChannels;
            float maxDelaySeconds = 0.001f;
            int lockDepth         = 0;
            long lockedCalls      = 0;

            void setPin(double t, uint8_t pin, bool level)
            {
                if (pin == noPin || levels[pin] == level) return;
                levels[pin] = level;
                recorded.push_back({t, pin, level});
            }

            // DIR of the motors that step at the next event of the channel gets its level now
            void prepareDirs(Chan& c)
            {
                if (c.queue.empty()) return;
                const Event& ev = c.queue.front().ev;
                for (unsigned i = 0; i < c.motors.size(); i++)
                    if ((ev.stepMask >> i) & 1) setPin(clock, c.motors[i].dir, (((ev.dirMask >> i) & 1) != 0) == c.motors[i].dirHighCountsUp);
            }

            void close(unsigned ch, bool notify)
            {
                Chan& c        = chans[ch];
                c.open         = false;
                c.running      = false;
                c.startPending = false;
                c.queue.clear();
                if (notify)
                {
                    if (lockDepth > 0) lockedCalls++;
                    finished(ch);
                }
            }

            // fetches events until the queue reaches far enough ahead (one event, without look-ahead)
            void refill(unsigned ch)
            {
                Chan& c = chans[ch];
                while (c.running && !c.fetchDone && (c.queue.empty() || c.lastQueued < clock + lookahead))
                {
                    if (lockDepth > 0) lockedCalls++;
                    Event ev;
                    if (!fetch(ch, ev))
                    {
                        c.fetchDone = true;
                        break;
                    }
                    c.lastQueued += ev.delay;
                    c.queue.push_back({c.lastQueued, ev});
                    if (lookahead > 0 && ev.arrived) arrived(ch); // buffered: when the event is queued
                }
                if (c.running && c.fetchDone && c.queue.empty())
                    close(ch, true);
                else if (c.running)
                    prepareDirs(c);
            }

            void execute(unsigned ch)
            {
                Chan& c  = chans[ch];
                Queued q = c.queue.front();
                c.queue.pop_front();
                for (unsigned i = 0; i < c.motors.size(); i++)
                {
                    if (!((q.ev.stepMask >> i) & 1)) continue;
                    const MotorPins& m = c.motors[i];
                    setPin(clock, m.step, m.stepActiveHigh);
                    if (m.step != noPin) pinChanges.push_back({clock + c.pulseWidth * 1e-6, m.step, !m.stepActiveHigh});
                }
                if (lookahead == 0 && q.ev.arrived) // one event per interrupt: after the steps, before the next fetch
                {
                    if (lockDepth > 0) lockedCalls++;
                    arrived(ch);
                }
                if (c.running) refill(ch);
            }

            // executes the next thing that is due up to time `until`; false if there is none
            bool executeNext(double until)
            {
                double t   = std::numeric_limits<double>::infinity();
                int change = -1, chan = -1;
                for (unsigned i = 0; i < pinChanges.size(); i++)
                    if (pinChanges[i].t < t) t = pinChanges[i].t, change = (int)i;
                for (unsigned ch = 0; ch < maxChannels; ch++)
                    if (chans[ch].running && !chans[ch].queue.empty() && chans[ch].queue.front().t < t) t = chans[ch].queue.front().t, chan = (int)ch, change = -1;
                if (t > until) return false;

                if (t > clock) clock = t;
                if (change >= 0)
                {
                    PinChange p = pinChanges[change];
                    pinChanges.erase(pinChanges.begin() + change);
                    setPin(clock, p.pin, p.level);
                }
                else
                {
                    execute((unsigned)chan);
                }
                if (lookahead > 0)
                    for (unsigned ch = 0; ch < maxChannels; ch++)
                        if (chans[ch].running) refill(ch);
                return true;
            }
        }

        //----------------------------------------------------------------------------------------------------
        // the backend interface

        float maxDelay() { return maxDelaySeconds; }

        void initMotor(const MotorPins& pins)
        {
            if (pins.step != noPin) levels[pins.step] = !pins.stepActiveHigh;
            if (pins.dir != noPin) levels[pins.dir] = pins.dirHighCountsUp;
        }

        void setPin(uint8_t pin, bool level)
        {
            if (pin == noPin || levels[pin] == level) return;
            levels[pin] = level;
            recorded.push_back({clock, pin, level});
        }

        errorCode open(unsigned channel, const ChannelSetup& setup)
        {
            if (host::openChannels() >= timers) return errorCode::noTimer;
            Chan& c      = chans[channel];
            c            = Chan();
            c.open       = true;
            c.pulseWidth = setup.pulseWidth;
            c.dirSetup   = setup.dirSetup;
            c.motors.assign(setup.motors, setup.motors + setup.count);
            return errorCode::OK;
        }

        void start(unsigned channel)
        {
            Chan& c = chans[channel];
            if (!c.open) return;
            if (c.running)
            {
                if (!c.fetchDone) return;
                c.fetchDone = false; // a new command came while the last fetched events are still being executed: go on
            }
            else
            {
                c.running    = true;
                c.fetchDone  = false;
                c.lastQueued = clock;
            }
            if (lockDepth > 0)
                c.startPending = true; // like an interrupt that is held back: fetch when the lock is released
            else
                refill(channel);
        }

        void halt(unsigned channel, int32_t* notExecuted)
        {
            Chan& c = chans[channel];
            for (unsigned i = 0; i < c.motors.size(); i++) notExecuted[i] = 0;
            for (const Queued& q : c.queue)
                for (unsigned i = 0; i < c.motors.size(); i++)
                    if ((q.ev.stepMask >> i) & 1) notExecuted[i] += ((q.ev.dirMask >> i) & 1) ? 1 : -1;
            close(channel, false);
        }

        void lock() { lockDepth++; }

        void unlock()
        {
            if (--lockDepth > 0) return;
            for (unsigned ch = 0; ch < maxChannels; ch++)
            {
                if (!chans[ch].startPending) continue;
                chans[ch].startPending = false;
                refill(ch);
            }
        }

        void idle() { host::run(0.001); }

        //----------------------------------------------------------------------------------------------------
        // control and inspection by the tests

        namespace host
        {
            void reset()
            {
                for (Chan& c : chans) c = Chan();
                for (bool& l : levels) l = false;
                recorded.clear();
                pinChanges.clear();
                clock           = 0;
                lookahead       = 0;
                timers          = maxChannels;
                maxDelaySeconds = 0.001f;
                lockDepth       = 0;
                lockedCalls     = 0;
            }

            void setLookahead(double seconds) { lookahead = seconds; }
            void setTimers(unsigned count) { timers = count; }
            void setMaxDelay(float seconds) { maxDelaySeconds = seconds; }
            double now() { return clock; }
            const std::vector<Edge>& edges() { return recorded; }
            bool level(uint8_t pin) { return levels[pin]; }
            long fetchesWhileLocked() { return lockedCalls; }

            unsigned openChannels()
            {
                unsigned n = 0;
                for (const Chan& c : chans) n += c.open;
                return n;
            }

            void run(double seconds)
            {
                double until = clock + seconds;
                while (executeNext(until)) {}
                clock = until;
                if (lookahead > 0)
                    for (unsigned ch = 0; ch < maxChannels; ch++)
                        if (chans[ch].running) refill(ch);
            }

            bool runUntilIdle(double maxSeconds)
            {
                double until = clock + maxSeconds;
                while (openChannels() > 0 || !pinChanges.empty())
                    if (!executeNext(until)) return openChannels() == 0 && pinChanges.empty();
                return true;
            }

            Pulses pulses(const MotorPins& motor, double since, double dirSetup)
            {
                Pulses p;
                bool dir = motor.dirHighCountsUp, step = !motor.stepActiveHigh; // levels set by initMotor()
                double dirChanged = -1, rise = 0;
                for (const Edge& e : recorded)
                {
                    if (e.pin == motor.dir && motor.dir != noPin)
                    {
                        dir        = e.level;
                        dirChanged = e.t;
                    }
                    if (e.pin != motor.step) continue;
                    step = e.level;
                    if (e.t < since) continue;
                    if (step == motor.stepActiveHigh) // a pulse begins
                    {
                        bool up = dir == motor.dirHighCountsUp;
                        if (p.count > 0)
                        {
                            double period = e.t - p.last;
                            if (p.count == 1 || period < p.minPeriod) p.minPeriod = period;
                            if (p.count == 1 || period > p.maxPeriod) p.maxPeriod = period;
                        }
                        else
                        {
                            p.first = e.t;
                        }
                        p.last = rise = e.t;
                        p.count++;
                        (up ? p.up : p.down)++;
                        if (dirChanged >= 0 && e.t - dirChanged < dirSetup) p.dirTooLate++;
                        p.times.push_back(e.t);
                    }
                    else if (p.count > 0) // a pulse ends
                    {
                        double width = e.t - rise;
                        if (p.minWidth == 0 || width < p.minWidth) p.minWidth = width;
                        if (width > p.maxWidth) p.maxWidth = width;
                    }
                }
                p.net = p.up - p.down;
                return p;
            }
        }
    }
}

#endif
