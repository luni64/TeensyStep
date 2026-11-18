// Helpers for the tests of core::Group: make a group, run it, check every step against the line.
#pragma once

#include "core/Group.h"
#include "sim.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace groupsim
{
    using namespace TeensyStep;
    using namespace TeensyStep::core;
    using namespace sim;

    struct Member
    {
        int32_t start;
        MotorLimits limits;
    };

    inline Group makeGroup(const std::vector<Member>& members)
    {
        Group g;
        REQUIRE(g.setMembers((unsigned)members.size()) == errorCode::OK);
        for (unsigned i = 0; i < members.size(); i++)
        {
            g.setLimits(i, members[i].limits);
            g.setPosition(i, members[i].start);
        }
        return g;
    }

    const MotorLimits fast{20'000, 50'000, 50'000, 0};
    const MotorLimits slow{5'000, 10'000, 10'000, 0};

    // Result of running a group. The checks that have to hold after every single step are made while it runs.
    struct GroupRun
    {
        Run lead;                // the motion along the path, for the checks of sim.h
        bool finished   = false; // the group stopped by itself
        long arrivals   = 0;     // events with the arrival flag
        long waitSteps  = 0;     // wait events that carried a step (must stay 0)
        double worstOff = 0;     // largest distance of a member from the line, in steps
        long tooFast    = 0;     // steps at which a member was above its maximum speed
        long wrongDir   = 0;     // steps with a direction bit that does not match the step made
    };

    // Runs a group. `line` gives, for every member, start position and the distance the member makes per
    // unit of the path; `pathNow` is the path position, in the same unit. For a move the unit is the whole
    // move (0 at the start, 1 at the targets), for a rotation it is one step of the fastest motor.
    template <class PathNow>
    GroupRun runGroup(Group& g, const std::vector<int32_t>& start, const std::vector<double>& perPath, PathNow pathNow,
                      const std::vector<MotorLimits>& limits, long maxSteps = 20'000'000, double maxTime = -1, bool untilArrival = false)
    {
        GroupRun r;
        unsigned n = (unsigned)start.size();
        std::vector<int32_t> prev(n);
        for (unsigned i = 0; i < n; i++) prev[i] = g.position(i);
        r.lead.v0   = g.lead().speed();
        r.lead.dir0 = r.lead.v0 > 0 ? 1 : (r.lead.v0 < 0 ? -1 : 0);

        double t = 0, tStep = 0;
        int32_t prevLead = g.lead().position();
        GroupEvent e;
        long events = 0;
        while (r.lead.steps() < maxSteps && (maxTime < 0 || t < maxTime))
        {
            if (++events > 60'000'000)
            {
                r.lead.runaway = true;
                break;
            }
            if (!g.next(e))
            {
                r.finished = true;
                break;
            }
            t += e.delay;
            r.lead.maxEventDelay = std::max(r.lead.maxEventDelay, (double)e.delay);
            if (e.arrived) r.arrivals++;

            int32_t leadNow = g.lead().position();
            if (leadNow == prevLead) // wait only
            {
                r.lead.waits++;
                if (e.stepMask != 0) r.waitSteps++;
                continue;
            }

            double path = pathNow();
            for (unsigned i = 0; i < n; i++)
            {
                int32_t x     = g.position(i);
                int32_t moved = x - prev[i];
                bool bit      = (e.stepMask >> i) & 1;
                if (bit != (moved != 0) || std::abs(moved) > 1) r.wrongDir++;                      // the mask says exactly which members step
                if (moved != 0 && (((e.dirMask >> i) & 1) != (moved > 0 ? 1u : 0u))) r.wrongDir++; // and in which direction
                double ideal = start[i] + perPath[i] * path;
                r.worstOff   = std::max(r.worstOff, std::fabs(x - ideal));
                if (std::fabs(g.speed(i)) > limits[i].maxSpeed * 1.0001) r.tooFast++;
                prev[i] = x;
            }
            r.lead.s.push_back({t, leadNow, g.lead().speed(), t - tStep, leadNow - prevLead, e.arrived});
            tStep    = t;
            prevLead = leadNow;
            if (untilArrival && e.arrived) break;
        }
        r.lead.time = t;
        return r;
    }

    // a move: the path runs from 0 at the start positions to 1 at the targets
    inline GroupRun runMove(Group& g, const std::vector<int32_t>& start, const std::vector<int32_t>& target, const std::vector<MotorLimits>& limits,
                     long maxSteps = 20'000'000, double maxTime = -1, bool untilArrival = false)
    {
        unsigned n = (unsigned)start.size();
        std::vector<double> dist(n);
        unsigned leadIdx = 0;
        for (unsigned i = 0; i < n; i++)
        {
            dist[i] = (double)target[i] - start[i];
            if (std::fabs(dist[i]) > std::fabs(dist[leadIdx])) leadIdx = i;
        }
        auto path = [&g, &start, &dist, leadIdx] { return dist[leadIdx] == 0 ? 0.0 : (g.position(leadIdx) - start[leadIdx]) / dist[leadIdx]; };
        return runGroup(g, start, dist, path, limits, maxSteps, maxTime, untilArrival);
    }

    // the limits that the path (the motor with the longest way) may use, so that no member exceeds its own
    inline Limits pathLimits(const std::vector<int32_t>& start, const std::vector<int32_t>& target, const std::vector<MotorLimits>& limits)
    {
        double longest = 0;
        for (unsigned i = 0; i < start.size(); i++) longest = std::max(longest, std::fabs((double)target[i] - start[i]));
        Limits L{1e9f, 1e9f, 1e9f, 1e9f};
        for (unsigned i = 0; i < start.size(); i++)
        {
            double ratio = std::fabs((double)target[i] - start[i]) / longest;
            if (ratio == 0) continue;
            L.vMax   = std::min(L.vMax, (float)(limits[i].maxSpeed / ratio));
            L.acc    = std::min(L.acc, (float)(limits[i].acceleration / ratio));
            L.dec    = std::min(L.dec, (float)(limits[i].deceleration / ratio));
            L.pullIn = std::min(L.pullIn, (float)(limits[i].pullInSpeed / ratio));
        }
        return L;
    }

    inline void checkOnLine(const GroupRun& r)
    {
        CHECK_MESSAGE(r.worstOff <= 0.5 + 1e-6, "a member was off the line by ", r.worstOff, " steps");
        CHECK_MESSAGE(r.wrongDir == 0, "step or direction bits that do not match the steps made: ", r.wrongDir);
        CHECK_MESSAGE(r.waitSteps == 0, "wait events with a step: ", r.waitSteps);
        CHECK_MESSAGE(r.tooFast == 0, "steps with a member above its maximum speed: ", r.tooFast);
    }
}
