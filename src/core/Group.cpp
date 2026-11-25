#include "Group.h"
#include "Hot.h"
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
            if (hasQueued) finish(queued);
        }

        void Group::setPosition(unsigned i, int32_t pos)
        {
            m[i].pos = pos;
        }

        /// The path may be as fast as every member allows. A member that makes only a part of the path's
        /// steps moves at that part of the path's speed, so it allows the path its own limit divided by its share.
        /// `want...` is what is asked for the path (0: nothing), `factor` the override of the speed.
        Group::PathLimits Group::limitsOf(const uint32_t* share, uint32_t len, float wantSpeed, float wantAcc, float wantDec, float factor) const
        {
            float vMax = Axis::maxSpeedLimit, acc = Axis::maxAccelerationLimit, dec = Axis::maxAccelerationLimit, pull = Axis::maxSpeedLimit;
            for (unsigned i = 0; i < count; i++)
            {
                if (share[i] == 0) continue; // a member that does not move sets no limit
                float perShare = (float)len / (float)share[i];
                vMax           = std::fmin(vMax, m[i].limits.maxSpeed * perShare);
                acc            = std::fmin(acc, m[i].limits.acceleration * perShare);
                dec            = std::fmin(dec, m[i].limits.deceleration * perShare);
                pull           = std::fmin(pull, m[i].limits.pullInSpeed * perShare);
            }
            // What is asked for, with the members as ceilings. A value that is the ceiling itself, calculated
            // by the caller with another rounding, is not "more than the members allow".
            const float same = 1.0001f;
            PathLimits l;
            l.speedCapped = wantSpeed > vMax * same;
            l.accCapped   = wantAcc > acc * same;
            l.decCapped   = wantDec > dec * same;
            if (wantSpeed > 0 && wantSpeed < vMax) vMax = wantSpeed;
            if (wantAcc > 0 && wantAcc < acc) acc = wantAcc;
            if (wantDec > 0 && wantDec < dec) dec = wantDec;
            l.vMax = vMax * factor;
            l.acc  = acc;
            l.dec  = dec;
            l.pull = pull;
            return l;
        }

        void Group::applyLimits()
        {
            uint32_t share[maxMembers];
            for (unsigned i = 0; i < count; i++) share[i] = m[i].share;
            bool own     = !(kind == Kind::move && pathMove); // a move of addMove() has its own values
            PathLimits l = limitsOf(share, length, own ? ownSpeed : moveSpeed, own ? ownAcc : moveAcc, own ? ownDec : moveAcc, own ? 1.0f : feed);
            speedCapped  = l.speedCapped;
            accCapped    = l.accCapped;
            decCapped    = l.decCapped;
            path.setMaxSpeed(l.vMax);
            path.setAcceleration(l.acc);
            path.setDeceleration(l.dec);
            path.setPullInSpeed(l.pull);
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
            hasQueued = false;
            pathMove  = false;
            stopping  = false;
            unit      = 1;
            moveExit  = speedAtTarget;
            serial++;

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
            moveExit       = speedAtTarget;
            errorCode code = path.moveAbs((int32_t)length, speedAtTarget);
            return code != errorCode::OK ? code : limitWarning();
        }

        //------------------------------------------------------------------------------------------------
        // a path that is given ahead

        /// Fills a line from `from` to `targets`. `entrySpeed`: the speed along the path at its start, in
        /// the unit of the caller of the move before (for the warning only).
        errorCode Group::prepare(Line& l, const int32_t* from, const int32_t* targets, const PathMove& move, float entrySpeed) const
        {
            uint32_t longest = 0;
            for (unsigned i = 0; i < count; i++)
            {
                int64_t d   = diff(targets[i], from[i]);
                l.dir[i]    = d < 0 ? -1 : 1;
                int64_t a   = d < 0 ? -d : d;
                l.share[i]  = a > INT32_MAX ? (uint32_t)INT32_MAX : (uint32_t)a;
                l.target[i] = targets[i];
                if (l.share[i] > longest) longest = l.share[i];
            }
            l.length = longest;
            if (longest == 0) return errorCode::OK;
            l.unit      = move.length > 0 ? (float)longest / move.length : 1.0f;
            l.wantSpeed = move.speed * l.unit;
            l.wantAcc   = move.acceleration * l.unit;
            l.exit      = move.speedAtTarget * l.unit;
            finish(l);
            if (l.speedCapped || l.exit * feed > l.limits.vMax * 1.0001f) return errorCode::speedLimited;
            if (l.accCapped) return errorCode::accelerationLimited;

            // Is the way long enough to get from the speed at the start to the speed at the targets? A plan
            // that uses exactly the whole way is long enough.
            PathLimits p  = limitsOf(l.share, l.length, l.wantSpeed, l.wantAcc, l.wantAcc, feed);
            double v0     = (double)entrySpeed * l.unit, vT = l.vT, pull = p.pull, way = longest;
            double from0  = v0 > pull ? v0 : pull, toT = vT > pull ? vT : pull;
            const double same = 1.002;
            bool tooShortToReach = vT > pull && vT * vT > (from0 * from0 + 2.0 * p.acc * way) * same;
            bool tooShortToSlow  = v0 > pull && v0 * v0 > (toT * toT + 2.0 * p.dec * way) * same;
            return tooShortToReach || tooShortToSlow ? errorCode::targetSpeedNotReached : errorCode::OK;
        }

        /// The limits of a line and its speed at the targets, with the limits of the members and the
        /// override as they are now.
        void Group::finish(Line& l) const
        {
            PathLimits p  = limitsOf(l.share, l.length, l.wantSpeed, l.wantAcc, l.wantAcc, feed);
            l.speedCapped = p.speedCapped;
            l.accCapped   = p.accCapped;
            l.limits      = Axis::makeLimits(p.vMax, p.acc, p.dec, p.pull);
            l.vT          = std::fmin(l.exit * feed, l.limits.vMax);
            l.wT          = Axis::squared(l.vT);
        }

        /// Starts a prepared line where the members are. Called from next() at an arrival: nothing here
        /// takes long.
        TEENSYSTEP_HOT void Group::activate(const Line& l)
        {
            float factor = l.unit / unit; // the speed along the path stays what it is in the unit of the caller
            forward      = 0;
            for (unsigned i = 0; i < count; i++)
            {
                Member& mem = m[i];
                mem.share   = l.share[i];
                mem.dir     = l.dir[i];
                mem.target  = l.target[i];
                mem.err     = l.length / 2;
                if (l.dir[i] > 0) forward |= (uint16_t)(1u << i);
            }
            length      = l.length;
            lastDirs    = forward;
            unit        = l.unit;
            moveSpeed   = l.wantSpeed;
            moveAcc     = l.wantAcc;
            moveExit    = l.exit;
            speedCapped = l.speedCapped;
            accCapped   = l.accCapped;
            decCapped   = l.accCapped;
            kind        = Kind::move;
            pathMove    = true;
            passed      = false;
            stopping    = false;
            serial++;
            path.setPosition(0);
            path.setLimits(l.limits);
            path.scaleSpeed(factor);
            path.startMove((int32_t)l.length, l.vT, l.wT);
        }

        errorCode Group::addMove(const int32_t* targets, const PathMove& move)
        {
            if (!(move.length >= 0) || !(move.speed >= 0) || !(move.speedAtTarget >= 0) || !(move.acceleration >= 0)) return errorCode::invalidArgument;
            bool moving = path.isMoving();
            if (moving && (kind != Kind::move || stopping)) return errorCode::notSupported;

            if (moving && !passed) // a move is on its way: this one waits
            {
                if (hasQueued) return errorCode::notSupported;
                int32_t from[maxMembers];
                for (unsigned i = 0; i < count; i++) from[i] = m[i].target;
                errorCode code = prepare(queued, from, targets, move, moveExit * feed / unit);
                if (queued.length == 0) return errorCode::invalidArgument;
                hasQueued = true;
                return code;
            }

            // the group stands still or runs on behind its last targets: the move starts where the members are
            int32_t from[maxMembers];
            for (unsigned i = 0; i < count; i++) from[i] = m[i].pos;
            Line l;
            errorCode code = prepare(l, from, targets, move, std::fabs(path.speed()) / unit);
            hasQueued      = false;
            if (l.length == 0) // all members are at their targets
            {
                if (!moving) return errorCode::OK;
                path.emergencyStop(); // they are there, but moving: nothing is left to slow down on
                return errorCode::targetSpeedNotReached;
            }
            if (!moving) unit = l.unit; // no speed to keep
            activate(l);
            return code;
        }

        bool Group::prepareWaiting(Line& line, const int32_t* targets, const PathMove& move, errorCode& code) const
        {
            if (!(move.length >= 0) || !(move.speed >= 0) || !(move.speedAtTarget >= 0) || !(move.acceleration >= 0)) return false;
            if (!path.isMoving() || kind != Kind::move || stopping || passed || hasQueued) return false;
            line.serial = serial;
            int32_t from[maxMembers];
            for (unsigned i = 0; i < count; i++) from[i] = m[i].target;
            code = prepare(line, from, targets, move, moveExit * feed / unit);
            if (line.length == 0) code = errorCode::invalidArgument;
            return true;
        }

        bool Group::commitWaiting(const Line& line)
        {
            // still the move the line was prepared for, still on its way, and none waits
            if (!path.isMoving() || kind != Kind::move || stopping || passed || hasQueued || line.serial != serial || line.length == 0) return false;
            queued    = line;
            hasQueued = true;
            return true;
        }

        unsigned Group::room() const
        {
            if (!path.isMoving()) return 2;
            if (kind != Kind::move || stopping) return 0;
            if (passed) return 2;
            return hasQueued ? 0 : 1;
        }

        errorCode Group::setOverride(float factor)
        {
            if (!(factor > 0 && factor <= 1)) return errorCode::invalidArgument;
            feed = factor;
            if (hasQueued) finish(queued);
            if (path.isMoving() && kind == Kind::move && pathMove && !stopping)
            {
                applyLimits();
                if (!passed) path.moveAbs((int32_t)length, moveExit * feed);
            }
            return errorCode::OK;
        }

        float Group::pathSpeed() const
        {
            return std::fabs(path.speed()) / unit;
        }

        errorCode Group::rotate(const float* speeds)
        {
            if (!path.isMoving())
            {
                hasQueued = false;
                pathMove  = false;
                stopping  = false;
                unit      = 1;
            }
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
            hasQueued = false;
            stopping  = true;
            return path.stop();
        }

        void Group::emergencyStop()
        {
            hasQueued = false;
            stopping  = false;
            path.emergencyStop();
        }

        //------------------------------------------------------------------------------------------------
        // events

        TEENSYSTEP_HOT bool Group::next(GroupEvent& event)
        {
            StepEvent e;
            if (!path.next(e)) return false;

            event.delay    = e.delay;
            event.arrived  = e.arrived;
            event.stepMask = 0;
            if (e.dir == 0) // wait only
            {
                event.dirMask = lastDirs;
                return true;
            }

            // Each member keeps `err` = share * path position + length / 2, modulo length. Whenever it passes
            // a multiple of length, the member has moved one more step along the line; it then steps. Going
            // backwards undoes exactly what going forwards did, so the members retrace the same positions.
            uint32_t mask      = 0;
            const uint32_t len = length;
            const unsigned n   = count;
            if (e.dir > 0)
            {
                for (unsigned i = 0; i < n; i++)
                {
                    Member& mem  = m[i];
                    uint32_t err = mem.err + mem.share;
                    if (err >= len)
                    {
                        err -= len;
                        mem.pos = (int32_t)((uint32_t)mem.pos + (uint32_t)(int32_t)mem.dir);
                        mask |= 1u << i;
                    }
                    mem.err = err;
                }
                event.dirMask = forward;
            }
            else
            {
                for (unsigned i = 0; i < n; i++)
                {
                    Member& mem = m[i];
                    if (mem.err < mem.share)
                    {
                        mem.err += len - mem.share;
                        mem.pos = (int32_t)((uint32_t)mem.pos - (uint32_t)(int32_t)mem.dir);
                        mask |= 1u << i;
                    }
                    else
                    {
                        mem.err -= mem.share;
                    }
                }
                event.dirMask = (uint16_t)(~forward & all);
            }
            event.stepMask = (uint16_t)mask;
            lastDirs       = event.dirMask;
            if (e.arrived)
            {
                if (hasQueued) // the path goes on with the move that waited
                {
                    hasQueued = false;
                    activate(queued);
                }
                else
                {
                    passed = true;
                }
            }
            return true;
        }

        float Group::speed(unsigned i) const
        {
            if (length == 0) return 0.0f;
            return path.speed() * ((float)m[i].share / (float)length) * (float)m[i].dir;
        }
    }
}
