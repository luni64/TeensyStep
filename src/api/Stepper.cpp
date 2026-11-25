#include "Stepper.h"
#include "Channels.h"

namespace TeensyStep
{
    using detail::Channel;
    using detail::Engine;
    using detail::Lock;

    namespace
    {
        float limited(float v, float lo, float hi, errorCode warning, errorCode& code)
        {
            if (!(v >= lo)) // also catches NaN
            {
                code = warning;
                return lo;
            }
            if (v > hi)
            {
                code = warning;
                return hi;
            }
            return v;
        }
    }

    //--------------------------------------------------------------------------------------------------------
    // construction and settings

    Stepper::Stepper(int stepPin, int dirPin)
    {
        pins.step = (uint8_t)stepPin;
        pins.dir  = (uint8_t)dirPin;
        backend::initMotor(pins);
    }

    Stepper::~Stepper()
    {
        Lock lock;
        if (channel >= 0) Engine::endNow((unsigned)channel);
    }

    /// Passes the motion settings on to a running move.
    void Stepper::applyLimits()
    {
        if (channel < 0) return;
        Channel& c = Engine::channels[channel];
        if (c.isGroup)
        {
            c.group.setLimits(member, Engine::limitsOf(*this, c));
            return;
        }
        c.axis.setMaxSpeed(vMax);
        c.axis.setAcceleration(acc);
        if (separateDeceleration) c.axis.setDeceleration(dec);
        c.axis.setPullInSpeed(pullIn);
    }

    Stepper& Stepper::setMaxSpeed(float v)
    {
        errorCode code = errorCode::OK;
        bool negative  = v < 0; // version 2: the sign is the direction of rotation for RotateControl
        v              = limited(negative ? -v : v, core::Axis::minSpeedLimit, core::Axis::maxSpeedLimit, errorCode::speedLimited, code);
        {
            Lock lock;
            vMax     = v;
            reversed = negative;
            applyLimits();
        }
        Engine::report(code);
        return *this;
    }

    Stepper& Stepper::setAcceleration(float a)
    {
        errorCode code = errorCode::OK;
        a              = limited(a, core::Axis::minAccelerationLimit, core::Axis::maxAccelerationLimit, errorCode::accelerationLimited, code);
        {
            Lock lock;
            acc = a;
            if (!separateDeceleration) dec = a;
            applyLimits();
        }
        Engine::report(code);
        return *this;
    }

    Stepper& Stepper::setDeceleration(float d)
    {
        errorCode code = errorCode::OK;
        d              = limited(d, core::Axis::minAccelerationLimit, core::Axis::maxAccelerationLimit, errorCode::accelerationLimited, code);
        {
            Lock lock;
            dec                  = d;
            separateDeceleration = true;
            applyLimits();
        }
        Engine::report(code);
        return *this;
    }

    Stepper& Stepper::setPullInSpeed(float v)
    {
        errorCode code = errorCode::OK;
        v              = limited(v, 0.0f, core::Axis::maxSpeedLimit, errorCode::speedLimited, code);
        {
            Lock lock;
            pullIn = v;
            applyLimits();
        }
        Engine::report(code);
        return *this;
    }

    Stepper& Stepper::setPullInOutSpeed(float pullInSpeed, float pullOutSpeed)
    {
        return setPullInSpeed(pullInSpeed < pullOutSpeed ? pullInSpeed : pullOutSpeed);
    }

    /// The settings of the hardware can only change while the motor has no channel.
    bool Stepper::hardwareSetting()
    {
        {
            Lock lock;
            if (channel >= 0 && Engine::channels[channel].ending) Engine::endNow((unsigned)channel); // only the drivers were waiting
            if (channel < 0) return true;
        }
        Engine::report(errorCode::notSupported);
        return false;
    }

    Stepper& Stepper::setPulseWidth(float microseconds)
    {
        if (hardwareSetting()) pulseWidth = microseconds > 0 ? microseconds : 0;
        return *this;
    }

    Stepper& Stepper::setDirSetupTime(float microseconds)
    {
        if (hardwareSetting()) dirSetup = microseconds > 0 ? microseconds : 0;
        return *this;
    }

    Stepper& Stepper::setStepPinPolarity(int activeLevel)
    {
        if (hardwareSetting())
        {
            pins.stepActiveHigh = activeLevel != 0;
            backend::initMotor(pins);
        }
        return *this;
    }

    Stepper& Stepper::setInverseRotation(bool inverse)
    {
        if (hardwareSetting())
        {
            pins.dirHighCountsUp = !inverse;
            backend::initMotor(pins);
        }
        return *this;
    }

    Stepper& Stepper::setEnablePin(int pin, int activeLevel, unsigned onDelayMs, unsigned offDelayMs)
    {
        if (hardwareSetting())
        {
            enablePin        = (uint8_t)pin;
            enableActiveHigh = activeLevel != 0;
            enableFunction   = nullptr;
            onDelay          = onDelayMs * 0.001f;
            offDelay         = offDelayMs * 0.001f;
            backend::setPin(enablePin, !enableActiveHigh); // driver off
        }
        return *this;
    }

    Stepper& Stepper::setEnableFunction(void (*function)(bool), unsigned onDelayMs, unsigned offDelayMs)
    {
        if (hardwareSetting())
        {
            enablePin      = backend::noPin;
            enableFunction = function;
            onDelay        = function ? onDelayMs * 0.001f : 0;
            offDelay       = function ? offDelayMs * 0.001f : 0;
            if (function) function(false); // driver off
        }
        return *this;
    }

    void Stepper::onArrival(void (*callback)())
    {
        Lock lock;
        arrival      = callback;
        arrivalMotor = nullptr;
    }

    void Stepper::onArrival(void (*callback)(Stepper&))
    {
        Lock lock;
        arrival      = nullptr;
        arrivalMotor = callback;
    }

    void Stepper::onArrival(std::nullptr_t)
    {
        Lock lock;
        arrival      = nullptr;
        arrivalMotor = nullptr;
    }

    void Stepper::callArrival()
    {
        if (arrival)
            arrival();
        else if (arrivalMotor)
            arrivalMotor(*this);
    }

    void Stepper::setPositionCounter(int32_t p)
    {
        Lock lock;
        if (channel < 0)
            pos = p;
        else if (Engine::channels[channel].isGroup)
            Engine::channels[channel].group.setPosition(member, p);
        else
            Engine::channels[channel].axis.setPosition(p);
    }

    void Stepper::setTargetAbs(int32_t p)
    {
        target    = p;
        hasTarget = true;
    }

    void Stepper::setTargetRel(int32_t delta)
    {
        target    = (int32_t)((uint32_t)getPosition() + (uint32_t)delta);
        hasTarget = true;
    }

    //--------------------------------------------------------------------------------------------------------
    // state

    int32_t Stepper::getPosition() const
    {
        Lock lock;
        if (channel < 0) return pos;
        const Channel& c = Engine::channels[channel];
        return c.isGroup ? c.group.position(member) : c.axis.position();
    }

    float Stepper::getSpeed() const
    {
        Lock lock;
        if (channel < 0) return 0.0f;
        const Channel& c = Engine::channels[channel];
        return c.isGroup ? c.group.speed(member) : c.axis.speed();
    }

    bool Stepper::isMoving() const
    {
        Lock lock;
        return Engine::isMoving(channel);
    }

    //--------------------------------------------------------------------------------------------------------
    // commands

    errorCode Stepper::command(Command cmd, int32_t p, float speed)
    {
        errorCode code    = errorCode::OK;
        bool alreadyThere = false;
        {
            Lock lock;
            Channel* c = channel >= 0 ? &Engine::channels[channel] : nullptr;

            if (c && c->isGroup)
            {
                if (!c->ending) // the motor is moving in a group: it takes no commands of its own, but a stop stops the group
                {
                    code = cmd == Command::stop ? c->group.stop() : errorCode::motorBusy;
                    goto done;
                }
                Engine::endNow((unsigned)channel); // the group has finished and only its drivers were waiting
                c = nullptr;
            }

            if (cmd == Command::moveRel)
            {
                int32_t here = c ? c->axis.position() : pos;
                p            = (int32_t)((uint32_t)here + (uint32_t)p);
                cmd          = Command::moveAbs;
            }

            if (cmd == Command::moveAbs && speed != 0.0f)
            {
                // The sign of the speed at the target is relative to the way the motor comes: positive passes
                // the target in the direction from the motor to the target, negative against it. The ramp
                // wants the direction itself. A motor that is at the target has no such way: the direction is
                // the one it moves in, and counting up if it stands still.
                int32_t way = (int32_t)((uint32_t)p - (uint32_t)(c ? c->axis.position() : pos));
                bool down   = way < 0 || (way == 0 && c && c->axis.speed() < 0.0f);
                if (down) speed = -speed;
            }

            if (!c) // the motor has no channel: it needs one, unless there is nothing to do
            {
                if (cmd == Command::stop || (cmd == Command::rotate && speed == 0.0f)) goto done;
                if (cmd == Command::moveAbs && speed == 0.0f && p == pos)
                {
                    alreadyThere = true;
                    goto done;
                }
                Stepper* self = this;
                int idx       = Engine::setup(&self, 1, false, 1.0f, false, code);
                if (idx < 0) goto done;
                c = &Engine::channels[idx];
            }

            c->ending = false; // a channel that only waited for its drivers moves again
            code      = cmd == Command::moveAbs ? c->axis.moveAbs(p, speed) : cmd == Command::rotate ? c->axis.rotate(speed) : c->axis.stop();
            backend::start((unsigned)channel);
        }
    done:
        if (alreadyThere) // no step is made, so the backend will not report the arrival
        {
            arrivals = arrivals + 1;
            callArrival();
        }
        return Engine::report(code);
    }

    errorCode Stepper::moveAbsAsync(int32_t p, float speedAtTarget) { return command(Command::moveAbs, p, speedAtTarget); }
    errorCode Stepper::moveRelAsync(int32_t delta, float speedAtTarget) { return command(Command::moveRel, delta, speedAtTarget); }
    errorCode Stepper::rotateAsync(float stepsPerSec) { return command(Command::rotate, 0, stepsPerSec); }
    errorCode Stepper::stopAsync() { return command(Command::stop, 0, 0); }

    void Stepper::emergencyStop()
    {
        Lock lock;
        if (channel >= 0) Engine::emergencyStop((unsigned)channel);
    }

    /// Waits for the end of a blocking move. A move that stops at its target has ended when the motor stands
    /// still and the last pulse is made. A move that passes its target at a speed has ended when the target
    /// is reported as reached (a buffered backend reports it when the step is queued, a little early).
    void Stepper::waitForArrival(uint32_t arrivalsBefore, float speedAtTarget)
    {
        if (speedAtTarget == 0.0f)
            while (isMoving()) backend::idle();
        else
            while (isMoving() && arrivals == arrivalsBefore) backend::idle();
    }

    errorCode Stepper::moveAbs(int32_t p, float speedAtTarget)
    {
        uint32_t before = arrivals;
        errorCode code  = moveAbsAsync(p, speedAtTarget);
        if (!isError(code)) waitForArrival(before, speedAtTarget);
        return code;
    }

    errorCode Stepper::moveRel(int32_t delta, float speedAtTarget)
    {
        uint32_t before = arrivals;
        errorCode code  = moveRelAsync(delta, speedAtTarget);
        if (!isError(code)) waitForArrival(before, speedAtTarget);
        return code;
    }

    errorCode Stepper::stop()
    {
        errorCode code = stopAsync();
        while (isMoving()) backend::idle();
        return code;
    }
}
