// Helpers for the tests of core::Axis: run an axis, record its steps, check the recorded motion.
#pragma once

#include "core/Axis.h"
#include "approx.h"
#include "doctest.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace sim
{
    using TeensyStep::core::Axis;
    using TeensyStep::core::StepEvent;

    struct Sample
    {
        double t;     // time of the step, seconds since the start of the run
        int32_t x;    // position after the step
        float v;      // speed after the step, as reported by the axis (0 if the step ended the motion)
        double dt;    // time since the previous step (or since the start of the run): the delays of the step
                      // event and of the wait events before it
        int dir;      // +1 / -1
        bool arrived; // the event carried the arrival flag
    };

    struct Run
    {
        std::vector<Sample> s;        // the steps
        bool finished        = false; // the axis stopped by itself (next() returned false)
        float v0             = 0;     // speed when the run started
        int dir0             = 0;     // direction of motion when the run started (0: standstill)
        long waits           = 0;     // events without a step
        double maxEventDelay = 0;     // longest delay of a single event
        double time          = 0;     // total time of all events
        bool endedInWait     = false; // the last event was a wait: the axis is in the middle of a step period
        bool startedMidStep  = false; // the run started in the middle of a step period (set by the caller)
        bool runaway         = false; // stopped because the axis produced an absurd number of events

        double duration() const { return s.empty() ? 0.0 : s.back().t; } // time of the last step
        long steps() const { return (long)s.size(); }

        float peakSpeed() const
        {
            float p = 0;
            for (auto& e : s) p = std::max(p, std::fabs(e.v));
            return p;
        }

        // number of changes of the direction of motion, including one against the motion before the run
        int reversals() const
        {
            int n = 0, d = dir0;
            for (auto& e : s)
            {
                if (d != 0 && e.dir != d) n++;
                d = e.dir;
            }
            return n;
        }

        // index of the first step with the arrival flag, -1 if none
        long arrival() const
        {
            for (size_t i = 0; i < s.size(); i++)
                if (s[i].arrived) return (long)i;
            return -1;
        }
    };

    enum class Until {
        finished, // until the axis stops by itself
        arrived,  // until (and including) the step with the arrival flag
        speed,    // until the axis has the speed `untilSpeed`
    };

    // Runs the axis for at most maxSteps steps, or (maxTime >= 0) until that much time has passed.
    inline Run run(Axis& a, long maxSteps = 20'000'000, Until until = Until::finished, double maxTime = -1, bool startedMidStep = false,
                   float untilSpeed = 0)
    {
        Run r;
        r.v0             = a.speed();
        r.dir0           = r.v0 > 0 ? 1 : (r.v0 < 0 ? -1 : 0);
        r.startedMidStep = startedMidStep;
        double t = 0, tStep = 0;
        StepEvent e;
        const long maxEvents = 30'000'000 + 4 * std::min(maxSteps, 20'000'000L); // far more than any test needs
        long events          = 0;
        while (r.steps() < maxSteps && (maxTime < 0 || t < maxTime))
        {
            if (++events > maxEvents)
            {
                r.runaway = true;
                break;
            }
            if (!a.next(e))
            {
                r.finished    = true;
                r.endedInWait = false;
                break;
            }
            t += e.delay;
            r.maxEventDelay = std::max(r.maxEventDelay, (double)e.delay);
            r.endedInWait   = e.dir == 0;
            if (e.dir == 0)
            {
                r.waits++;
                continue;
            }
            r.s.push_back({t, a.position(), a.speed(), t - tStep, e.dir, e.arrived});
            tStep = t;
            if (until == Until::arrived && e.arrived) break;
            if (until == Until::speed && a.speed() == untilSpeed) break;
        }
        r.time = t;
        return r;
    }

    // Runs the axis for a time; it may end in the middle of a step period.
    inline Run runFor(Axis& a, double seconds, bool startedMidStep = false)
    {
        return run(a, 20'000'000, Until::finished, seconds, startedMidStep);
    }

    struct Limits
    {
        float vMax, acc, dec, pullIn;
    };

    // Checks a recorded run against the limits. Speeds are compared through their squares, because the
    // core accelerates by a fixed amount of v² per step; the tolerance covers the float speed getter.
    inline void checkKinematics(const Run& r, const Limits& L)
    {
        long badStep = 0, badSpeed = 0, badAcc = 0, badDec = 0, badStop = 0, badDelay = 0, badTiming = 0;
        double prevV  = std::fabs(r.v0);
        int prevDir   = r.dir0;
        int32_t prevX = r.s.empty() ? 0 : r.s.front().x - r.s.front().dir;
        const double wp = (double)L.pullIn * L.pullIn;
        bool first      = true;

        for (auto& e : r.s)
        {
            double v = std::fabs(e.v);
            if (e.x - prevX != e.dir || (e.dir != 1 && e.dir != -1)) badStep++;
            if (v > L.vMax * 1.0001) badSpeed++;
            if (!(e.dt > 0) || !std::isfinite(e.dt)) badDelay++;

            // If the speed changes during the whole step, the step takes the time of one step at the mean of the
            // speeds before and after it. If a limit is reached within the step, the speed changes during a part
            // of it only; the step is then shorter, but not shorter than one step at the higher of the two speeds.
            // Not checked for a step whose period had already begun when the run (and its command) started.
            if (!(first && r.startedMidStep))
            {
                double vBefore = (prevDir == 0 || prevDir == e.dir) ? prevV : 0.0; // after a reversal the axis starts from standstill
                vBefore        = std::max(vBefore, std::min((double)L.pullIn, v));  // below the pull-in speed it starts at that speed
                double vHigh   = std::max({vBefore, v, v == 0 ? (double)L.pullIn : 0.0});
                if (vBefore + v > 0)
                {
                    double whole = 2.0 / (vBefore + v);
                    if (e.dt > whole * 1.001 || e.dt < 1.0 / vHigh * 0.999) badTiming++;

                    double rate    = v >= vBefore ? 2.0 * L.acc : 2.0 * L.dec;
                    double change  = std::fabs(v * v - vBefore * vBefore);
                    bool wholeStep = v > 0 && std::fabs(change - rate) < 1e-3 * rate + 1e-6 * std::max(v * v, vBefore * vBefore);
                    if (wholeStep && std::fabs(e.dt / whole - 1.0) > 1e-3) badTiming++;
                }
            }

            double tol   = 1e-6 * std::max(v * v, prevV * prevV) + 1e-3;
            double dw    = v * v - std::max(prevV * prevV, std::min(wp, v * v)); // counted from the pull-in speed
            bool sameDir = prevDir == 0 || prevDir == e.dir;
            if (sameDir && v > 0)
            {
                if (dw > 2.0 * L.acc * 1.001 + tol) badAcc++;                                 // at most 2a per step
                if (prevV * prevV > wp * 1.0001 && dw < -2.0 * L.dec * 1.001 - tol) badDec++; // from above pull-in: at most 2d per step
            }
            if (v == 0 || !sameDir) // the step ended the motion, or the direction changed: only from a low speed
            {
                double stopMax = wp + 2.0 * L.dec;
                if (prevV * prevV > stopMax * 1.001 + tol) badStop++;
            }
            prevV   = v;
            prevDir = v == 0 ? 0 : e.dir;
            prevX   = e.x;
            first   = false;
        }
        CHECK_MESSAGE(!r.runaway, "the run was stopped: too many events");
        CHECK_MESSAGE(badStep == 0, "steps that are not exactly one step: ", badStep);
        CHECK_MESSAGE(badSpeed == 0, "steps above the maximum speed: ", badSpeed);
        CHECK_MESSAGE(badAcc == 0, "steps above the acceleration: ", badAcc);
        CHECK_MESSAGE(badDec == 0, "steps above the deceleration: ", badDec);
        CHECK_MESSAGE(badStop == 0, "stops or reversals from a speed that is too high: ", badStop);
        CHECK_MESSAGE(badDelay == 0, "delays that are not positive and finite: ", badDelay);
        CHECK_MESSAGE(badTiming == 0, "delays that do not match the speeds: ", badTiming);
    }

    // Duration of a move over `dist` steps from standstill to standstill with constant acceleration a
    // (acceleration = deceleration, no pull-in speed): trapezoid or triangle profile.
    inline double idealDuration(double dist, double vMax, double a)
    {
        double sAcc = vMax * vMax / (2 * a);
        if (2 * sAcc <= dist) return 2 * vMax / a + (dist - 2 * sAcc) / vMax;
        return 2 * std::sqrt(dist / a);
    }
}
