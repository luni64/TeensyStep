#include "Channels.h"
#include "Stepper.h"

namespace TeensyStep
{
    namespace
    {
        void (*errFunc)(errorCode) = nullptr;

        // Number of running channels that use an enable pin. Several motors can share one enable signal; the
        // drivers stay on as long as one of them is in use.
        constexpr unsigned enablePins = 64;
        uint8_t enableUse[enablePins];

        float largest(float a, float b) { return a > b ? a : b; }
    }

    void attachErrFunc(void (*handler)(errorCode))
    {
        errFunc = handler;
    }

    void emergencyStopAll()
    {
        detail::Lock lock;
        for (unsigned ch = 0; ch < backend::maxChannels; ch++) detail::Engine::emergencyStop(ch);
    }

    namespace detail
    {
        Channel Engine::channels[backend::maxChannels];

        errorCode Engine::report(errorCode code)
        {
            if (code != errorCode::OK && errFunc) errFunc(code);
            return code;
        }

        /// The limits of a motor as a member of the channel's group.
        core::MotorLimits Engine::limitsOf(const Stepper& s, const Channel& c)
        {
            core::MotorLimits l;
            l.maxSpeed     = c.unlimitedSpeed ? core::Axis::maxSpeedLimit : s.vMax * c.speedFactor;
            l.acceleration = s.acc;
            l.deceleration = s.dec;
            l.pullInSpeed  = s.pullIn;
            return l;
        }

        bool Engine::isMoving(int channel)
        {
            if (channel < 0) return false;
            const Channel& c = channels[channel];
            return !(c.ending && c.offDelay > 0); // while only the drivers wait to be switched off, the motors stand still
        }

        int Engine::setup(Stepper* const* motors, unsigned count, bool isGroup, float speedFactor, bool unlimitedSpeed, errorCode& code)
        {
            int idx = -1;
            for (unsigned i = 0; i < backend::maxChannels && idx < 0; i++)
                if (!channels[i].used) idx = (int)i;
            if (idx < 0)
            {
                code = errorCode::noTimer;
                return -1;
            }

            Channel& c       = channels[idx];
            c                = Channel();
            c.isGroup        = isGroup;
            c.count          = (uint8_t)count;
            c.speedFactor    = speedFactor;
            c.unlimitedSpeed = unlimitedSpeed;
            c.pulseWidth     = 0;
            c.dirSetup       = 0;
            for (unsigned i = 0; i < count; i++)
            {
                c.motors[i]  = motors[i];
                c.pins[i]    = motors[i]->pins;
                c.pulseWidth = largest(c.pulseWidth, motors[i]->pulseWidth);
                c.dirSetup   = largest(c.dirSetup, motors[i]->dirSetup);
            }

            backend::ChannelSetup setup;
            setup.motors     = c.pins;
            setup.count      = count;
            setup.pulseWidth = c.pulseWidth;
            setup.dirSetup   = c.dirSetup;
            code             = backend::open((unsigned)idx, setup);
            if (code != errorCode::OK) return -1;
            c.used = true;

            for (unsigned i = 0; i < count; i++)
            {
                Stepper* s = motors[i];
                s->channel = (int8_t)idx;
                s->member  = (uint8_t)i;
                if (s->enablePin >= enablePins) continue;
                c.offDelay   = largest(c.offDelay, s->offDelay);
                c.startDelay = largest(c.startDelay, s->onDelay); // also if another motor has just switched the shared signal on
                if (enableUse[s->enablePin]++ == 0) backend::setPin(s->enablePin, s->enableActiveHigh);
            }

            if (isGroup)
            {
                c.group.setMembers(count);
                c.group.setMaxDelay(backend::maxDelay());
                for (unsigned i = 0; i < count; i++)
                {
                    c.group.setLimits(i, limitsOf(*motors[i], c));
                    c.group.setPosition(i, motors[i]->pos);
                }
            }
            else
            {
                Stepper* s = motors[0];
                c.axis.setMaxSpeed(s->vMax);
                c.axis.setAcceleration(s->acc);
                if (s->separateDeceleration) c.axis.setDeceleration(s->dec);
                c.axis.setPullInSpeed(s->pullIn);
                c.axis.setMaxDelay(backend::maxDelay());
                c.axis.setPosition(s->pos);
            }
            return idx;
        }

        void Engine::release(unsigned ch, const int32_t* notExecuted)
        {
            Channel& c = channels[ch];
            if (!c.used) return;
            for (unsigned i = 0; i < c.count; i++)
            {
                Stepper* s   = c.motors[i];
                int32_t here = c.isGroup ? c.group.position(i) : c.axis.position();
                s->pos       = (int32_t)((uint32_t)here - (uint32_t)(notExecuted ? notExecuted[i] : 0));
                s->channel   = -1;
                if (s->enablePin < enablePins && enableUse[s->enablePin] > 0 && --enableUse[s->enablePin] == 0)
                    backend::setPin(s->enablePin, !s->enableActiveHigh);
            }
            c.used = false;
        }

        void Engine::endNow(unsigned ch)
        {
            int32_t notExecuted[backend::maxMotors];
            backend::halt(ch, notExecuted); // steps that were calculated but not made do not count
            release(ch, notExecuted);
        }

        void Engine::emergencyStop(unsigned ch)
        {
            Channel& c = channels[ch];
            if (!c.used) return;
            if (c.offDelay <= 0)
            {
                endNow(ch);
                return;
            }

            // The drivers shall stay on for their off-delay: stop the motion, keep the channel, and let it
            // run the off-delay only.
            int32_t notExecuted[backend::maxMotors];
            backend::halt(ch, notExecuted);
            if (c.isGroup)
            {
                c.group.emergencyStop();
                for (unsigned i = 0; i < c.count; i++) c.group.setPosition(i, (int32_t)((uint32_t)c.group.position(i) - (uint32_t)notExecuted[i]));
            }
            else
            {
                c.axis.emergencyStop();
                c.axis.setPosition((int32_t)((uint32_t)c.axis.position() - (uint32_t)notExecuted[0]));
            }
            c.ending     = true;
            c.offLeft    = c.offDelay;
            c.startDelay = 0;

            backend::ChannelSetup setup;
            setup.motors     = c.pins;
            setup.count      = c.count;
            setup.pulseWidth = c.pulseWidth;
            setup.dirSetup   = c.dirSetup;
            if (backend::open(ch, setup) == errorCode::OK)
                backend::start(ch);
            else
                release(ch, nullptr); // no timer for the delay: the drivers go off now
        }

        //----------------------------------------------------------------------------------------------------
        // called by the backend

        bool Engine::fetch(unsigned ch, backend::Event& event)
        {
            Channel& c = channels[ch];
            if (!c.used) return false;
            float maxDelay = backend::maxDelay();
            event.stepMask = 0;
            event.dirMask  = 0;
            event.arrived  = false;

            if (!c.ending)
            {
                if (c.startDelay > 0) // the drivers are switching on
                {
                    event.delay = c.startDelay < maxDelay ? c.startDelay : maxDelay;
                    c.startDelay -= event.delay;
                    return true;
                }
                bool more;
                if (c.isGroup)
                {
                    core::GroupEvent e;
                    more = c.group.next(e);
                    if (more)
                    {
                        event.delay    = e.delay;
                        event.stepMask = e.stepMask;
                        event.dirMask  = e.dirMask;
                        event.arrived  = e.arrived;
                    }
                }
                else
                {
                    core::StepEvent e;
                    more = c.axis.next(e);
                    if (more)
                    {
                        event.delay    = e.delay;
                        event.stepMask = e.dir != 0 ? 1 : 0;
                        event.dirMask  = e.dir > 0 ? 1 : 0;
                        event.arrived  = e.arrived;
                    }
                }
                if (more) return true;
                c.ending  = true; // the move is over
                c.offLeft = c.offDelay;
            }

            if (c.offLeft > 0) // the drivers stay on for a while
            {
                event.delay = c.offLeft < maxDelay ? c.offLeft : maxDelay;
                c.offLeft -= event.delay;
                if (c.offLeft < 1e-7f) c.offLeft = 0;
                return true;
            }
            return false;
        }

        void Engine::arrived(unsigned ch)
        {
            Channel& c = channels[ch];
            if (!c.used) return;
            if (c.isGroup)
            {
                c.motors[0]->arrivals = c.motors[0]->arrivals + 1; // for the blocking moves of the group
                if (c.arrival) c.arrival();
                return;
            }
            Stepper* s  = c.motors[0];
            s->arrivals = s->arrivals + 1;
            if (s->arrival) s->arrival();
        }

        void Engine::finished(unsigned ch)
        {
            release(ch, nullptr);
        }
    }

    bool backend::fetch(unsigned channel, Event& event) { return detail::Engine::fetch(channel, event); }
    void backend::arrived(unsigned channel) { detail::Engine::arrived(channel); }
    void backend::finished(unsigned channel) { detail::Engine::finished(channel); }
}
