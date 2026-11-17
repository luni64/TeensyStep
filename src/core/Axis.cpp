#include "Axis.h"
#include <cmath>

namespace TeensyStep
{
    namespace core
    {
        namespace
        {
            constexpr double wOne  = 1048576.0;             // 2^20: fixed point scale of the squared speeds
            constexpr int64_t wBig = (int64_t)1 << 61;      // "no limit"; sums of two such values still fit

            int64_t toW(float v) // speed -> squared speed
            {
                return (int64_t)std::llround((double)v * (double)v * wOne);
            }

            int64_t toDelta(float a) // acceleration -> change of the squared speed per step (2a)
            {
                return (int64_t)std::llround(2.0 * (double)a * wOne);
            }

            float toSpeed(int64_t w) // squared speed -> speed
            {
                return std::sqrt((float)w * (float)(1.0 / wOne));
            }

            int32_t diff(int32_t a, int32_t b) // a - b, the short way around the 32-bit range
            {
                return (int32_t)((uint32_t)a - (uint32_t)b);
            }

            int8_t sign(int64_t v) { return (v > 0) - (v < 0); }

            float limited(float v, float lo, float hi, bool& changed)
            {
                if (!(v >= lo)) // also catches NaN
                {
                    changed = true;
                    return lo;
                }
                if (v > hi)
                {
                    changed = true;
                    return hi;
                }
                return v;
            }
        }

        Axis::Axis()
        {
            setMaxSpeed(800);
            setAcceleration(2500);
            setPullInSpeed(100);
        }

        //------------------------------------------------------------------------------------------------
        // limits

        errorCode Axis::setMaxSpeed(float v)
        {
            bool changed = false;
            vMax         = limited(v, minSpeedLimit, maxSpeedLimit, changed);
            wMax         = toW(vMax);
            version++;
            return changed ? errorCode::speedLimited : errorCode::OK;
        }

        errorCode Axis::setAcceleration(float a)
        {
            bool changed = false;
            twoA         = toDelta(limited(a, minAccelerationLimit, maxAccelerationLimit, changed));
            maxStepsA    = wBig / twoA;
            version++;
            if (!separateDeceleration)
            {
                twoD      = twoA;
                maxStepsD = maxStepsA;
            }
            return changed ? errorCode::accelerationLimited : errorCode::OK;
        }

        errorCode Axis::setDeceleration(float d)
        {
            bool changed         = false;
            twoD                 = toDelta(limited(d, minAccelerationLimit, maxAccelerationLimit, changed));
            maxStepsD            = wBig / twoD;
            separateDeceleration = true;
            version++;
            return changed ? errorCode::accelerationLimited : errorCode::OK;
        }

        errorCode Axis::setPullInSpeed(float v)
        {
            bool changed = false;
            vPull        = limited(v, 0.0f, maxSpeedLimit, changed);
            wPull        = toW(vPull);
            version++;
            return changed ? errorCode::speedLimited : errorCode::OK;
        }

        void Axis::setMaxDelay(float seconds)
        {
            maxDelay = seconds >= 0.00001f ? seconds : 0.00001f;
        }

        //------------------------------------------------------------------------------------------------
        // commands

        errorCode Axis::moveAbs(int32_t pos, float speedAtTarget)
        {
            bool changed = false;
            float v      = std::fabs(speedAtTarget);
            if (v != 0.0f) v = limited(v, minSpeedLimit, vMax, changed);

            version++;
            vTarget = v;
            xT      = pos;
            wT      = v != 0.0f ? toW(v) : 0;
            if (wT > wMax) wT = wMax;
            sT = v == 0.0f ? 0 : (speedAtTarget > 0 ? 1 : -1);

            bool alreadyThere = sigma == 0 && sT == 0 && diff(xT, x) == 0;
            mode              = alreadyThere ? Mode::idle : Mode::position;
            return changed ? errorCode::speedLimited : errorCode::OK;
        }

        errorCode Axis::moveRel(int32_t delta, float speedAtTarget)
        {
            return moveAbs((int32_t)((uint32_t)x + (uint32_t)delta), speedAtTarget);
        }

        errorCode Axis::rotate(float speed)
        {
            if (speed == 0.0f) return stop();
            bool changed = false;
            float v      = limited(std::fabs(speed), minSpeedLimit, vMax, changed);
            version++;
            vGoal = v;
            wG    = toW(v);
            if (wG > wMax) wG = wMax;
            sG   = speed > 0 ? 1 : -1;
            mode = Mode::velocity;
            return changed ? errorCode::speedLimited : errorCode::OK;
        }

        errorCode Axis::stop()
        {
            version++;
            wG    = 0;
            sG    = 0;
            vGoal = 0;
            mode = sigma == 0 ? Mode::idle : Mode::velocity;
            return errorCode::OK;
        }

        void Axis::emergencyStop()
        {
            standStill();
            mode    = Mode::idle;
            pending = false;
            version++;
        }

        void Axis::standStill()
        {
            w     = 0;
            sigma = 0;
            vCur  = 0;
        }

        //------------------------------------------------------------------------------------------------
        // the next event

        bool Axis::next(StepEvent& event)
        {
            if (pending && pendVersion != version)
            {
                // A command or a limit changed while the step was waiting: decide again. The part of the
                // step period that has passed counts for the new step, if it goes in the same direction.
                float done = pendDone + (1.0f - pendDone) * (1.0f - (float)pendLeft / (float)pendPieces);
                pending    = false;
                if (!decide(done, pend.dir)) return false;
            }
            else if (!pending)
            {
                if (!decide(0.0f, 0)) return false;
            }

            event.delay = pendPiece;
            if (pendLeft > 1) // wait only
            {
                pendLeft--;
                event.dir     = 0;
                event.arrived = false;
                return true;
            }

            // the last event of the period makes the step
            pending       = false;
            event.dir     = pend.dir;
            event.arrived = pend.arrived;

            x     = (int32_t)((uint32_t)x + (uint32_t)(int32_t)pend.dir);
            w     = pend.w;
            sigma = pend.dir;
            vCur  = pendV;
            if (pend.thenStop) standStill();
            if (pend.arrived)
            {
                if (sT == 0)
                {
                    mode = Mode::idle;
                }
                else // keep the speed of the target
                {
                    mode  = Mode::velocity;
                    wG    = wT;
                    sG    = sT;
                    vGoal = vTarget;
                }
            }
            return true;
        }

        /// The speed for a squared speed. The speeds given by the sketch are returned as given, not as the
        /// root of their rounded square, so that they are kept exactly also at very low speeds.
        float Axis::speedOf(int64_t wNew) const
        {
            if (wNew == 0) return 0.0f;
            if (wNew == wMax) return vMax;
            if (mode == Mode::velocity && wNew == wG) return vGoal;
            if (mode == Mode::position && wNew == wT) return vTarget;
            return toSpeed(wNew);
        }

        /// Decides the next step and plans the events of its period. doneBefore is the part of a period that
        /// has already passed for a step in direction dirBefore (0: none).
        /// @return false if there is no step to make; the axis then stands still and is idle
        bool Axis::decide(float doneBefore, int8_t dirBefore)
        {
            for (int i = 0; i < 4; i++) // a halt is followed by a new decision; never more than two in a row
            {
                Plan p = mode == Mode::position ? planPosition()
                         : mode == Mode::velocity ? planVelocity()
                                                  : Plan{Plan::idle, 0, 0, false, false};
                if (p.kind == Plan::idle)
                {
                    standStill();
                    mode = Mode::idle;
                    return false;
                }
                if (p.kind == Plan::halt)
                {
                    standStill();
                    continue;
                }

                float vNew, period;
                if (p.w == w && p.dir == sigma) // constant speed: same period as for the last step
                {
                    vNew = vCur;
                    if (wDelay != w)
                    {
                        wDelay = w;
                        delay  = 1.0f / vCur;
                    }
                    period = delay;
                }
                else
                {
                    vNew = speedOf(p.w);

                    // Below the pull-in speed the axis changes its speed without a ramp: it starts the step
                    // with the pull-in speed (or with the new speed, if that is lower still).
                    float vFrom   = vCur;
                    int64_t wFrom = w;
                    if (w < wPull)
                    {
                        if (p.w < wPull)
                        {
                            if (p.w > wFrom) wFrom = p.w, vFrom = vNew;
                        }
                        else
                        {
                            wFrom = wPull, vFrom = vPull;
                        }
                    }

                    // While the speed changes linearly with time, the mean speed is the mean of the speeds at
                    // both ends. If the change needs only a part of the step (a limit is reached), the rest of
                    // the step runs at constant speed: at the new speed after speeding up, at the old one
                    // before slowing down.
                    int64_t change = p.w - wFrom;
                    int64_t rate   = change >= 0 ? twoA : twoD;
                    if (change < 0) change = -change;
                    if (vFrom == 0.0f && vNew == 0.0f) // a single step from standstill to standstill: half a step up, half a step down
                    {
                        period = 1.4142136f * (1.0f / toSpeed(twoA) + 1.0f / toSpeed(twoD));
                    }
                    else if (change >= rate)
                    {
                        period = 2.0f / (vFrom + vNew);
                    }
                    else
                    {
                        float part = (float)change / (float)rate;
                        period     = part * 2.0f / (vFrom + vNew) + (1.0f - part) / (p.w >= wFrom ? vNew : vFrom);
                    }
                }

                pend     = p;
                pendV    = vNew;
                pendDone = p.dir == dirBefore ? doneBefore : 0.0f;

                float rest = period * (1.0f - pendDone);
                if (rest <= maxDelay)
                {
                    pendPieces = 1;
                    pendPiece  = rest;
                }
                else // equal pieces, none longer than the maximum delay
                {
                    float n    = std::ceil(rest / maxDelay);
                    pendPieces = n < 2.0e9f ? (uint32_t)n : 2000000000u;
                    pendPiece  = rest / (float)pendPieces;
                }
                pendLeft    = pendPieces;
                pendVersion = version;
                pending     = true;
                return true;
            }
            emergencyStop(); // not reachable
            return false;
        }

        /// The squared speed after one more step when the axis goes as fast as it may, up to `limit`.
        int64_t Axis::faster(int64_t limit) const
        {
            if (w > limit) // too fast (the limit was lowered): slow down
            {
                if (w <= wPull) return limit; // up to the pull-in speed no ramp is needed
                int64_t slower = w - twoD;
                return slower > limit ? slower : limit;
            }
            int64_t up = (w < wPull ? wPull : w) + twoA; // below the pull-in speed: start at that speed
            return up < limit ? up : limit;
        }

        //------------------------------------------------------------------------------------------------
        // velocity mode: reach the speed sG * sqrt(wG) and keep it

        Axis::Plan Axis::planVelocity()
        {
            int64_t limit = wG < wMax ? wG : wMax;
            if (sigma == 0)
            {
                if (sG == 0) return {Plan::idle, 0, 0, false, false};
                return {Plan::step, sG, faster(limit), false, false};
            }
            if (sigma == sG) return {Plan::step, sigma, faster(limit), false, false};

            return brake(); // wrong direction, or stop
        }

        //------------------------------------------------------------------------------------------------
        // position mode

        Axis::Plan Axis::planPosition()
        {
            if (sT == 0) return approachStop(xT, true);

            // Target with a speed. It can be approached directly if the axis moves towards it in the
            // direction of that speed and can still reach the speed. Otherwise the axis first goes to the
            // point from which the run-up fits exactly, and stops there.
            int64_t wTe = wT < wMax ? wT : wMax;
            int64_t rho = (int64_t)sT * diff(xT, x); // distance to the target, along the direction at the target
            if ((sigma == sT || sigma == 0) && rho >= 1 && feasible(w, rho, wTe)) return stepToMovingTarget(rho, wTe);

            int32_t runUp = (int32_t)((uint32_t)xT - (uint32_t)(int32_t)(sT * runUpSteps(wTe)));
            return approachStop(runUp, false);
        }

        /// Can the axis, moving towards a target in rho steps, have the squared speed wTe there?
        bool Axis::feasible(int64_t w0, int64_t rho, int64_t wTe) const
        {
            int64_t arrive = wTe > wPull - twoD ? wTe : wPull - twoD; // see stepToMovingTarget
            bool slowEnough = w0 <= wPull || w0 - timesTwoD(rho) <= arrive;
            int64_t first   = (w0 < wPull ? wPull : w0) + twoA; // squared speed after the first step
            bool fastEnough = wTe <= wPull || first + timesTwoA(rho - 1) >= wTe;
            return slowEnough && fastEnough;
        }

        /// Steps needed from standstill to reach the squared speed wTe.
        int64_t Axis::runUpSteps(int64_t wTe) const
        {
            int64_t first = wPull + twoA; // squared speed after the first step
            if (wTe <= first) return 1;
            int64_t n = 1 + (wTe - first + twoA - 1) / twoA;
            return n < ((int64_t)1 << 30) ? n : ((int64_t)1 << 30);
        }

        /// One step towards a target that is passed with the squared speed wTe; rho steps are left.
        Axis::Plan Axis::stepToMovingTarget(int64_t rho, int64_t wTe)
        {
            // After this step, rho - 1 steps remain, each of which can take 2d off the squared speed. The
            // last one may also drop from the pull-in speed to any lower speed.
            int64_t arrive = wTe > wPull - twoD ? wTe : wPull - twoD;
            int64_t cap    = arrive + timesTwoD(rho - 1);
            int64_t wNew   = faster(wMax);
            if (wNew > cap) wNew = cap;
            if (rho == 1) wNew = wTe;
            return {Plan::step, sT, wNew, rho == 1, false};
        }

        /// One step towards the point p, where the axis shall stand still: the commanded target (isMain) or
        /// the start of a run-up. If the axis moves away from p or is too fast to stop there, it brakes first.
        Axis::Plan Axis::approachStop(int32_t p, bool isMain)
        {
            int64_t q  = diff(p, x);
            int8_t dir = sigma;
            if (dir == 0)
            {
                if (q == 0) return {Plan::idle, 0, 0, false, false};
                dir = sign(q);
            }

            int64_t rho = dir * q; // steps to p in the direction of motion
            if (rho >= 1 && (w <= wPull || w - timesTwoD(rho) <= wPull))
            {
                // after this step, rho - 1 steps remain to get down to the pull-in speed, from which the axis stops
                int64_t cap  = wPull + timesTwoD(rho - 1);
                int64_t wNew = faster(wMax);
                if (wNew > cap) wNew = cap;
                return {Plan::step, dir, wNew, isMain && rho == 1, rho == 1};
            }

            return brake();
        }

        /// Decelerates: one step slower, or standstill once the axis is slow enough.
        Axis::Plan Axis::brake()
        {
            // Standstill from the pull-in speed, or when the braking distance is below half a step.
            if (w <= wPull || 2 * w <= twoD) return {Plan::halt, 0, 0, false, false};
            int64_t wNew = w > twoD ? w - twoD : 0;
            return {Plan::step, sigma, wNew, false, wNew == 0};
        }
    }
}
