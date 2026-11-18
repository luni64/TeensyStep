#include "Group.h"
#include <cmath>

namespace TeensyStep
{
    namespace core
    {
        namespace
        {
            constexpr uint32_t rotationLength = (uint32_t)1 << 30; // resolution of the speed ratios of a rotating group

            int32_t diff(int32_t a, int32_t b) // a - b, the short way around the 32-bit range
            {
                return (int32_t)((uint32_t)a - (uint32_t)b);
            }
        }

        errorCode Group::setMembers(unsigned n)
        {
            if (n < 1 || n > maxMembers) return errorCode::invalidArgument;
            if (path.isMoving()) return errorCode::notSupported;
            count = n;
            all   = (uint16_t)(((uint32_t)1 << n) - 1);
            for (unsigned i = 0; i < maxMembers; i++) m[i] = Member();
            kind   = Kind::none;
            length = 0;
            return errorCode::OK;
        }

        void Group::setLimits(unsigned i, const MotorLimits& limits)
        {
            m[i].limits = limits;
            if (kind != Kind::none && length > 0) applyLimits();
        }

        void Group::setPosition(unsigned i, int32_t pos)
        {
            m[i].pos = pos;
        }

        /// The path may be as fast as every member allows. A member that makes only a part of the path's
        /// steps moves at that part of the path's speed, so it allows the path its own limit divided by its share.
        void Group::applyLimits()
        {
            float vMax = Axis::maxSpeedLimit, acc = Axis::maxAccelerationLimit, dec = Axis::maxAccelerationLimit, pull = Axis::maxSpeedLimit;
            for (unsigned i = 0; i < count; i++)
            {
                if (m[i].share == 0) continue; // a member that does not move sets no limit
                float perShare = (float)length / (float)m[i].share;
                vMax           = std::fmin(vMax, m[i].limits.maxSpeed * perShare);
                acc            = std::fmin(acc, m[i].limits.acceleration * perShare);
                dec            = std::fmin(dec, m[i].limits.deceleration * perShare);
                pull           = std::fmin(pull, m[i].limits.pullInSpeed * perShare);
            }
            // what the group asks for, with the members as ceilings
            speedCapped = ownSpeed > vMax;
            accCapped   = ownAcc > acc;
            decCapped   = ownDec > dec;
            if (ownSpeed > 0 && !speedCapped) vMax = ownSpeed;
            if (ownAcc > 0 && !accCapped) acc = ownAcc;
            if (ownDec > 0 && !decCapped) dec = ownDec;

            path.setMaxSpeed(vMax);
            path.setAcceleration(acc);
            path.setDeceleration(dec);
            path.setPullInSpeed(pull);
        }

        errorCode Group::limitWarning() const
        {
            return speedCapped ? errorCode::speedLimited : (accCapped || decCapped) ? errorCode::accelerationLimited : errorCode::OK;
        }

        errorCode Group::setMaxSpeed(float v)
        {
            ownSpeed = v > 0 ? v : 0;
            if (kind == Kind::none || length == 0) return errorCode::OK;
            applyLimits();
            return speedCapped ? errorCode::speedLimited : errorCode::OK;
        }

        errorCode Group::setAcceleration(float a)
        {
            ownAcc = a > 0 ? a : 0;
            if (!separateDeceleration) ownDec = ownAcc;
            if (kind == Kind::none || length == 0) return errorCode::OK;
            applyLimits();
            return accCapped || (!separateDeceleration && decCapped) ? errorCode::accelerationLimited : errorCode::OK;
        }

        errorCode Group::setDeceleration(float d)
        {
            ownDec               = d > 0 ? d : 0;
            separateDeceleration = true;
            if (kind == Kind::none || length == 0) return errorCode::OK;
            applyLimits();
            return decCapped ? errorCode::accelerationLimited : errorCode::OK;
        }

        void Group::startPath(uint32_t pathLength)
        {
            length  = pathLength;
            forward = 0;
            for (unsigned i = 0; i < count; i++)
            {
                m[i].err = length / 2; // a member steps when it is half a step behind
                if (m[i].dir > 0) forward |= (uint16_t)(1u << i);
            }
            lastDirs = forward;
            path.setPosition(0);
            applyLimits();
        }

        //------------------------------------------------------------------------------------------------
        // commands

        errorCode Group::moveAbs(const int32_t* targets, float speedAtTarget, float speedAtStart)
        {
            bool keep = speedAtStart != speedAtStart; // keepSpeed
            if (!(speedAtTarget >= 0) || !(keep || speedAtStart >= 0)) return errorCode::invalidArgument;

            bool moving = path.isMoving();
            if (moving)
            {
                if (kind != Kind::move) return errorCode::notSupported;
                if (!passed) // on the way: only the speed at the target can change
                {
                    for (unsigned i = 0; i < count; i++)
                        if (targets[i] != m[i].target) return errorCode::notSupported;
                    return setSpeedAtTarget(speedAtTarget);
                }
            }

            // A new line, from where the members are. If the group runs on behind its last targets, this
            // is the next segment: the path goes on without a stop.
            uint32_t longest = 0;
            for (unsigned i = 0; i < count; i++)
            {
                int64_t d   = diff(targets[i], m[i].pos);
                m[i].dir    = d < 0 ? -1 : 1;
                int64_t a   = d < 0 ? -d : d;
                m[i].share  = a > INT32_MAX ? (uint32_t)INT32_MAX : (uint32_t)a;
                m[i].target = targets[i];
                if (m[i].share > longest) longest = m[i].share;
            }
            kind   = Kind::move;
            passed = false;
            if (longest == 0) // all members are at their targets
            {
                length = 0;
                if (!moving) return errorCode::OK;
                path.emergencyStop(); // they are there, but moving: nothing is left to slow down on
                return errorCode::targetSpeedNotReached;
            }
            startPath(longest);
            if (!keep) path.setSpeed(speedAtStart);
            errorCode code = path.moveAbs((int32_t)longest, speedAtTarget);
            return code != errorCode::OK ? code : limitWarning();
        }

        errorCode Group::setSpeedAtTarget(float speedAtTarget)
        {
            if (!(speedAtTarget >= 0)) return errorCode::invalidArgument;
            if (!path.isMoving() || kind != Kind::move || passed) return errorCode::notSupported;
            errorCode code = path.moveAbs((int32_t)length, speedAtTarget);
            return code != errorCode::OK ? code : limitWarning();
        }

        errorCode Group::rotate(const float* speeds)
        {
            float fastest = 0;
            for (unsigned i = 0; i < count; i++) fastest = std::fmax(fastest, std::fabs(speeds[i]));
            if (fastest == 0.0f) return stop();

            uint32_t share[maxMembers];
            int8_t dir[maxMembers];
            for (unsigned i = 0; i < count; i++)
            {
                share[i] = (uint32_t)std::llround((double)std::fabs(speeds[i]) / (double)fastest * (double)rotationLength);
                dir[i]   = speeds[i] < 0 ? -1 : 1;
            }

            if (path.isMoving())
            {
                // Only the speed of the group as a whole can change: same ratio, all directions kept or all reversed.
                if (kind != Kind::rotate) return errorCode::notSupported;
                int sense = 0;
                for (unsigned i = 0; i < count; i++)
                {
                    uint32_t delta = share[i] > m[i].share ? share[i] - m[i].share : m[i].share - share[i];
                    if (delta > 1) return errorCode::notSupported;
                    if (share[i] == 0) continue;
                    int s = dir[i] == m[i].dir ? 1 : -1;
                    if (sense != 0 && s != sense) return errorCode::notSupported;
                    sense = s;
                }
                baseSpeed = fastest * (float)(sense < 0 ? -1 : 1);
                return path.rotate(baseSpeed);
            }

            for (unsigned i = 0; i < count; i++)
            {
                m[i].share = share[i];
                m[i].dir   = dir[i];
            }
            kind      = Kind::rotate;
            baseSpeed = fastest;
            startPath(rotationLength);
            return path.rotate(baseSpeed);
        }

        errorCode Group::overrideSpeed(float factor)
        {
            if (kind != Kind::rotate) return errorCode::notSupported;
            return path.rotate(factor * baseSpeed);
        }

        errorCode Group::stop()
        {
            return path.stop();
        }

        void Group::emergencyStop()
        {
            path.emergencyStop();
        }

        //------------------------------------------------------------------------------------------------
        // events

        bool Group::next(GroupEvent& event)
        {
            StepEvent e;
            if (!path.next(e)) return false;

            event.delay    = e.delay;
            event.arrived  = e.arrived;
            event.stepMask = 0;
            if (e.arrived) passed = true;
            if (e.dir == 0) // wait only
            {
                event.dirMask = lastDirs;
                return true;
            }

            // Each member keeps `err` = share * path position + length / 2, modulo length. Whenever it passes
            // a multiple of length, the member has moved one more step along the line; it then steps. Going
            // backwards undoes exactly what going forwards did, so the members retrace the same positions.
            uint16_t mask = 0;
            if (e.dir > 0)
            {
                for (unsigned i = 0; i < count; i++)
                {
                    Member& mem = m[i];
                    mem.err += mem.share;
                    if (mem.err >= length)
                    {
                        mem.err -= length;
                        mem.pos = (int32_t)((uint32_t)mem.pos + (uint32_t)(int32_t)mem.dir);
                        mask |= (uint16_t)(1u << i);
                    }
                }
                event.dirMask = forward;
            }
            else
            {
                for (unsigned i = 0; i < count; i++)
                {
                    Member& mem = m[i];
                    if (mem.err < mem.share)
                    {
                        mem.err += length - mem.share;
                        mem.pos = (int32_t)((uint32_t)mem.pos - (uint32_t)(int32_t)mem.dir);
                        mask |= (uint16_t)(1u << i);
                    }
                    else
                    {
                        mem.err -= mem.share;
                    }
                }
                event.dirMask = (uint16_t)(~forward & all);
            }
            event.stepMask = mask;
            lastDirs       = event.dirMask;
            return true;
        }

        float Group::speed(unsigned i) const
        {
            if (length == 0) return 0.0f;
            return path.speed() * ((float)m[i].share / (float)length) * (float)m[i].dir;
        }
    }
}
