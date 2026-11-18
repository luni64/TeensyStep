#include "StepperGroup.h"
#include "Channels.h"

namespace TeensyStep
{
    using detail::Channel;
    using detail::Engine;
    using detail::Lock;

    StepperGroup::StepperGroup(std::initializer_list<std::reference_wrapper<Stepper>> list)
    {
        for (Stepper& m : list) add(m);
    }

    errorCode StepperGroup::add(Stepper& motor)
    {
        bool known = false;
        for (unsigned i = 0; i < count; i++) known = known || motors[i] == &motor;
        if (known || count >= maxMembers) return Engine::report(errorCode::invalidArgument);
        motors[count++] = &motor;
        hasBaseSpeed    = false;
        return errorCode::OK;
    }

    void StepperGroup::remove(Stepper& motor)
    {
        unsigned kept = 0;
        for (unsigned i = 0; i < count; i++)
            if (motors[i] != &motor) motors[kept++] = motors[i];
        count        = kept;
        hasBaseSpeed = false;
    }

    void StepperGroup::clear()
    {
        count        = 0;
        hasBaseSpeed = false;
    }

    /// The channel in which exactly these motors move as a group, or -1. To be called with the lock held.
    int StepperGroup::runningChannel() const
    {
        if (count == 0 || motors[0]->channel < 0) return -1;
        int ch           = motors[0]->channel;
        const Channel& c = Engine::channels[ch];
        if (!c.isGroup || c.count != count) return -1;
        for (unsigned i = 0; i < count; i++)
            if (c.motors[i] != motors[i]) return -1;
        return ch;
    }

    /// Makes sure that the group has a channel. To be called with the lock held.
    errorCode StepperGroup::takeChannel(int& ch)
    {
        ch = runningChannel();
        if (ch >= 0) return errorCode::OK;

        for (unsigned i = 0; i < count; i++)
        {
            int other = motors[i]->channel;
            if (other < 0) continue;
            if (!Engine::channels[other].ending) return errorCode::motorBusy; // the member is moving on its own or in another group
            Engine::endNow((unsigned)other);                                  // only its drivers were waiting
        }
        errorCode code = errorCode::OK;
        ch             = Engine::setup(motors, count, true, speedFactor, unlimitedSpeed, code);
        return code;
    }

    //--------------------------------------------------------------------------------------------------------
    // speed and acceleration of the group

    /// Gives the values of this group object to the ramp of a channel, and reports a restriction of the
    /// value that was changed. To be called with the lock held.
    void StepperGroup::applySettings(Channel& c, Setting changed)
    {
        errorCode speed = c.group.setMaxSpeed(ownSpeed);
        errorCode acc   = c.group.setAcceleration(ownAcc);
        errorCode dec   = c.group.setDeceleration(separateDeceleration ? ownDec : ownAcc);
        if (changed == Setting::speed) Engine::report(speed);
        if (changed == Setting::acceleration) Engine::report(acc != errorCode::OK || separateDeceleration ? acc : dec);
        if (changed == Setting::deceleration) Engine::report(dec);
    }

    StepperGroup& StepperGroup::setMaxSpeed(float stepsPerSec)
    {
        Lock lock;
        ownSpeed = stepsPerSec > 0 ? stepsPerSec : 0;
        int ch   = runningChannel();
        if (ch >= 0) applySettings(Engine::channels[ch], Setting::speed);
        return *this;
    }

    StepperGroup& StepperGroup::setAcceleration(float stepsPerSec2)
    {
        Lock lock;
        ownAcc = stepsPerSec2 > 0 ? stepsPerSec2 : 0;
        int ch = runningChannel();
        if (ch >= 0) applySettings(Engine::channels[ch], Setting::acceleration);
        return *this;
    }

    StepperGroup& StepperGroup::setDeceleration(float stepsPerSec2)
    {
        Lock lock;
        ownDec               = stepsPerSec2 > 0 ? stepsPerSec2 : 0;
        separateDeceleration = true;
        int ch               = runningChannel();
        if (ch >= 0) applySettings(Engine::channels[ch], Setting::deceleration);
        return *this;
    }

    float StepperGroup::getSpeed() const
    {
        Lock lock;
        int ch = runningChannel();
        if (ch < 0 || Engine::channels[ch].ending) return 0.0f;
        float v = Engine::channels[ch].group.lead().speed();
        return v < 0 ? -v : v;
    }

    //--------------------------------------------------------------------------------------------------------
    // moves

    /// A move to targets. The moves of the group stop at the targets (speedAtTarget 0) and start at the
    /// speed the group has; advanced::GroupPath gives both speeds.
    errorCode StepperGroup::start(const int32_t* targets, unsigned n, float speedAtTarget, float speedAtStart)
    {
        if (n != count || count == 0) return Engine::report(errorCode::invalidArgument);
        errorCode code    = errorCode::OK;
        bool alreadyThere = false;
        {
            Lock lock;
            alreadyThere = true;
            for (unsigned i = 0; i < count; i++) alreadyThere = alreadyThere && motors[i]->channel < 0 && motors[i]->pos == targets[i];
            if (!alreadyThere)
            {
                int ch;
                code = takeChannel(ch);
                if (code == errorCode::OK)
                {
                    Channel& c = Engine::channels[ch];
                    applySettings(c, Setting::none); // a restriction by the members is reported by the move
                    code = c.group.moveAbs(targets, speedAtTarget, speedAtStart); // notSupported for other targets while the group is on its way
                    if (!isError(code))
                    {
                        c.ending  = false;
                        c.arrival = arrival;
                        backend::start((unsigned)ch);
                    }
                }
            }
            if (!isError(code)) // the members remember the targets; a refused command changes nothing
                for (unsigned i = 0; i < count; i++) motors[i]->setTargetAbs(targets[i]);
        }
        if (alreadyThere && arrival) arrival();
        return Engine::report(code);
    }

    errorCode StepperGroup::startRel(const int32_t* distances, unsigned n, float speedAtTarget, float speedAtStart)
    {
        if (n != count || count == 0) return Engine::report(errorCode::invalidArgument);
        int32_t targets[maxMembers];
        for (unsigned i = 0; i < count; i++) targets[i] = (int32_t)((uint32_t)motors[i]->getPosition() + (uint32_t)distances[i]);
        return start(targets, count, speedAtTarget, speedAtStart);
    }

    errorCode StepperGroup::changeSpeedAtTarget(float speedAtTarget)
    {
        errorCode code = errorCode::notSupported; // if the group is not moving
        {
            Lock lock;
            int ch = runningChannel();
            if (ch >= 0 && !Engine::channels[ch].ending) code = Engine::channels[ch].group.setSpeedAtTarget(speedAtTarget);
        }
        return Engine::report(code);
    }

    errorCode StepperGroup::moveAbsAsync(const int32_t* targets, unsigned n)
    {
        return start(targets, n, 0, core::Group::keepSpeed);
    }

    errorCode StepperGroup::moveAbsAsync(std::initializer_list<int32_t> targets)
    {
        return moveAbsAsync(targets.begin(), (unsigned)targets.size());
    }

    errorCode StepperGroup::moveAsync()
    {
        int32_t targets[maxMembers];
        for (unsigned i = 0; i < count; i++) targets[i] = motors[i]->hasTarget ? motors[i]->target : motors[i]->getPosition();
        return moveAbsAsync(targets, count);
    }

    errorCode StepperGroup::moveRelAsync(const int32_t* distances, unsigned n)
    {
        return startRel(distances, n, 0, core::Group::keepSpeed);
    }

    errorCode StepperGroup::moveRelAsync(std::initializer_list<int32_t> distances)
    {
        return moveRelAsync(distances.begin(), (unsigned)distances.size());
    }

    errorCode StepperGroup::moveRel(const int32_t* distances, unsigned n)
    {
        errorCode code = moveRelAsync(distances, n);
        if (!isError(code))
            while (isMoving()) backend::idle();
        return code;
    }

    errorCode StepperGroup::moveRel(std::initializer_list<int32_t> distances)
    {
        return moveRel(distances.begin(), (unsigned)distances.size());
    }

    errorCode StepperGroup::moveAbs(const int32_t* targets, unsigned n)
    {
        errorCode code = moveAbsAsync(targets, n);
        if (!isError(code))
            while (isMoving()) backend::idle();
        return code;
    }

    errorCode StepperGroup::moveAbs(std::initializer_list<int32_t> targets)
    {
        return moveAbs(targets.begin(), (unsigned)targets.size());
    }

    errorCode StepperGroup::move()
    {
        errorCode code = moveAsync();
        if (!isError(code))
            while (isMoving()) backend::idle();
        return code;
    }

    //--------------------------------------------------------------------------------------------------------
    // rotation

    errorCode StepperGroup::rotate(const float* speeds, unsigned n, bool remember)
    {
        if (n != count || count == 0) return Engine::report(errorCode::invalidArgument);
        errorCode code = errorCode::OK;
        {
            Lock lock;
            bool allZero = true;
            for (unsigned i = 0; i < count; i++) allZero = allZero && speeds[i] == 0.0f;

            int ch = runningChannel();
            if (allZero)
            {
                if (ch >= 0) code = Engine::channels[ch].group.stop();
            }
            else
            {
                code = takeChannel(ch);
                if (code == errorCode::OK)
                {
                    Channel& c = Engine::channels[ch];
                    code       = c.group.rotate(speeds); // notSupported for another ratio while the group is moving
                    if (!isError(code))
                    {
                        c.ending  = false;
                        c.arrival = arrival;
                        backend::start((unsigned)ch);
                    }
                }
            }
            if (remember && !isError(code))
            {
                for (unsigned i = 0; i < count; i++) baseSpeed[i] = speeds[i];
                hasBaseSpeed = !allZero;
            }
        }
        return Engine::report(code);
    }

    errorCode StepperGroup::rotateAsync(const float* speeds, unsigned n)
    {
        return rotate(speeds, n, true);
    }

    errorCode StepperGroup::rotateAsync(std::initializer_list<float> speeds)
    {
        return rotate(speeds.begin(), (unsigned)speeds.size(), true);
    }

    errorCode StepperGroup::overrideSpeed(float factor)
    {
        if (!hasBaseSpeed) return Engine::report(errorCode::notSupported); // the group has not been rotated
        float speeds[maxMembers];
        for (unsigned i = 0; i < count; i++) speeds[i] = baseSpeed[i] * factor;
        return rotate(speeds, count, false);
    }

    //--------------------------------------------------------------------------------------------------------
    // stop and state

    errorCode StepperGroup::stopAsync()
    {
        errorCode code = errorCode::OK;
        {
            Lock lock;
            int ch = runningChannel();
            if (ch >= 0) code = Engine::channels[ch].group.stop();
        }
        return Engine::report(code);
    }

    errorCode StepperGroup::stop()
    {
        errorCode code = stopAsync();
        while (isMoving()) backend::idle();
        return code;
    }

    void StepperGroup::emergencyStop()
    {
        Lock lock;
        int ch = runningChannel();
        if (ch >= 0) Engine::emergencyStop((unsigned)ch);
    }

    bool StepperGroup::isMoving() const
    {
        Lock lock;
        return Engine::isMoving(runningChannel());
    }
}
