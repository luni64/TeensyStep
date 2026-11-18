// core::Group: speed and acceleration of a group, moves with a speed at the target, and segments that follow
// each other without a stop.
#include "groupsim.h"

#include <random>

using namespace TeensyStep;
using namespace TeensyStep::core;
using namespace sim;
using namespace groupsim;

namespace
{
    // speed of the path at the step with the arrival flag
    float speedAtArrival(const GroupRun& r)
    {
        long i = r.lead.arrival();
        REQUIRE(i >= 0);
        return r.lead.s[(size_t)i].v;
    }

    std::vector<int32_t> positions(const Group& g)
    {
        std::vector<int32_t> p;
        for (unsigned i = 0; i < g.members(); i++) p.push_back(g.position(i));
        return p;
    }
}

// ------------------------------------------------------------------------------------------- speed and acceleration of the group

TEST_CASE("group speed: the group runs at its speed, measured at the member with the longest way")
{
    std::vector<int32_t> start{0, 0}, target{40'000, 10'000};
    std::vector<MotorLimits> lim{fast, fast};
    Group g = makeGroup({{0, fast}, {0, fast}});
    CHECK(g.setMaxSpeed(8'000) == errorCode::OK);
    CHECK(g.moveAbs(target.data()) == errorCode::OK);
    GroupRun part = runMove(g, start, target, lim, 20'000);
    CHECK(g.speed(0) == approx(8'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(2'000).epsilon(0.0001));
    GroupRun r = runMove(g, start, target, lim);
    CHECK(r.finished);
    CHECK(positions(g) == target);
    CHECK(part.lead.peakSpeed() == approx(8'000).epsilon(0.0001));
    checkOnLine(r);
}

TEST_CASE("group speed: the limits of the members are ceilings")
{
    std::vector<int32_t> start{0, 0}, target{40'000, 20'000}; // the slow member allows 10000 on the path
    std::vector<MotorLimits> lim{fast, slow};

    SUBCASE("a group speed above them is limited and reported")
    {
        Group g = makeGroup({{0, fast}, {0, slow}});
        CHECK(g.setMaxSpeed(15'000) == errorCode::OK); // no move yet: nothing to compare with
        CHECK(g.moveAbs(target.data()) == errorCode::speedLimited);
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.lead.peakSpeed() == approx(10'000).epsilon(0.0001));
        CHECK(positions(g) == target);
        checkOnLine(r);
        checkKinematics(r.lead, pathLimits(start, target, lim));
    }
    SUBCASE("a group speed below them is used")
    {
        Group g = makeGroup({{0, fast}, {0, slow}});
        g.setMaxSpeed(4'000);
        CHECK(g.moveAbs(target.data()) == errorCode::OK);
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.lead.peakSpeed() == approx(4'000).epsilon(0.0001));
        Limits L = pathLimits(start, target, lim);
        L.vMax   = 4'000;
        checkKinematics(r.lead, L);
    }
    SUBCASE("a member limit that is lowered during the move still wins")
    {
        Group g = makeGroup({{0, fast}, {0, slow}});
        g.setMaxSpeed(8'000);
        g.moveAbs(target.data());
        runMove(g, start, target, lim, 15'000);
        CHECK(g.lead().speed() == approx(8'000).epsilon(0.0001));
        MotorLimits slower = slow;
        slower.maxSpeed    = 1'000; // 2000 on the path
        g.setLimits(1, slower);
        runMove(g, start, target, lim, 10'000);
        CHECK(g.lead().speed() == approx(2'000).epsilon(0.0001));
    }
    SUBCASE("speed 0 removes the speed of the group")
    {
        Group g = makeGroup({{0, fast}, {0, slow}});
        g.setMaxSpeed(4'000);
        g.setMaxSpeed(0);
        CHECK(g.moveAbs(target.data()) == errorCode::OK);
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.lead.peakSpeed() == approx(10'000).epsilon(0.0001));
    }
}

TEST_CASE("group speed: it can change during a move")
{
    std::vector<int32_t> start{0, 0}, target{200'000, 50'000};
    std::vector<MotorLimits> lim{fast, fast};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.setMaxSpeed(5'000);
    g.moveAbs(target.data());
    runMove(g, start, target, lim, 20'000);
    CHECK(g.lead().speed() == approx(5'000).epsilon(0.0001));

    CHECK(g.setMaxSpeed(12'000) == errorCode::OK);
    runMove(g, start, target, lim, 20'000);
    CHECK(g.lead().speed() == approx(12'000).epsilon(0.0001));

    CHECK(g.setMaxSpeed(3'000) == errorCode::OK);
    runMove(g, start, target, lim, 20'000);
    CHECK(g.lead().speed() == approx(3'000).epsilon(0.0001));

    CHECK(g.setMaxSpeed(50'000) == errorCode::speedLimited); // the members allow 20000
    GroupRun r = runMove(g, start, target, lim);
    CHECK(r.lead.peakSpeed() == approx(20'000).epsilon(0.0001));
    CHECK(positions(g) == target);
    checkOnLine(r);
}

TEST_CASE("group acceleration: used for the path, with the members as ceilings")
{
    std::vector<int32_t> start{0, 0}, target{40'000, 10'000};
    std::vector<MotorLimits> lim{fast, fast}; // 20000 steps/s, 50000 steps/s²

    SUBCASE("lower than the members allow")
    {
        Group g = makeGroup({{0, fast}, {0, fast}});
        CHECK(g.setAcceleration(10'000) == errorCode::OK);
        CHECK(g.moveAbs(target.data()) == errorCode::OK);
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.lead.duration() == approx(idealDuration(40'000, 20'000, 10'000)).epsilon(0.002));
        checkKinematics(r.lead, {20'000, 10'000, 10'000, 0});
        checkOnLine(r);
    }
    SUBCASE("a separate deceleration")
    {
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.setAcceleration(40'000);
        g.setDeceleration(8'000);
        CHECK(g.moveAbs(target.data()) == errorCode::OK);
        GroupRun r = runMove(g, start, target, lim);
        double expected = 20'000.0 / 40'000 + 20'000.0 / 8'000 + (40'000 - 5'000 - 25'000) / 20'000.0;
        CHECK(r.lead.duration() == approx(expected).epsilon(0.002));
        checkKinematics(r.lead, {20'000, 40'000, 8'000, 0});
    }
    SUBCASE("higher than the members allow: limited and reported")
    {
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.setAcceleration(200'000);
        CHECK(g.moveAbs(target.data()) == errorCode::accelerationLimited);
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.lead.duration() == approx(idealDuration(40'000, 20'000, 50'000)).epsilon(0.002));
        checkKinematics(r.lead, pathLimits(start, target, lim));
    }
    SUBCASE("changed during the move")
    {
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.moveAbs(target.data());
        runMove(g, start, target, lim, 20'000);
        CHECK(g.setDeceleration(100'000) == errorCode::accelerationLimited);
        CHECK(g.setDeceleration(5'000) == errorCode::OK); // braking now needs 40000 steps: the group overshoots and returns
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.finished);
        CHECK(positions(g) == target);
        checkOnLine(r);
    }
}

TEST_CASE("group speed and acceleration also limit a rotating group")
{
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.setMaxSpeed(3'000);
    g.setAcceleration(6'000);
    float speeds[] = {10'000, -5'000};
    CHECK(g.rotate(speeds) == errorCode::speedLimited);
    double tUp = 0;
    GroupEvent e;
    while (g.lead().speed() < 2'999.9f)
    {
        REQUIRE(g.next(e));
        tUp += e.delay;
    }
    CHECK(tUp == approx(0.5).epsilon(0.01)); // 3000 / 6000
    CHECK(g.speed(0) == approx(3'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(-1'500).epsilon(0.0001));
}

// ------------------------------------------------------------------------------------------- a speed at the target

TEST_CASE("group target with a speed: the group passes its targets at the speed and runs on along the line")
{
    std::vector<int32_t> start{0, 100, -50}, target{40'000, 100 - 13'337, -50 + 77};
    std::vector<MotorLimits> lim{fast, fast, fast};
    Group g = makeGroup({{start[0], fast}, {start[1], fast}, {start[2], fast}});

    CHECK(g.moveAbs(target.data(), 3'000) == errorCode::OK);
    GroupRun r = runMove(g, start, target, lim, 20'000'000, -1, true);
    CHECK(r.arrivals == 1);
    CHECK(r.lead.s.back().arrived);
    CHECK(positions(g) == target); // all members are at their targets at this step
    CHECK(speedAtArrival(r) == approx(3'000).epsilon(0.0001));
    CHECK(r.lead.peakSpeed() == approx(20'000).epsilon(0.0001));
    CHECK(r.lead.reversals() == 0);
    CHECK(g.isMoving());
    checkOnLine(r);
    checkKinematics(r.lead, pathLimits(start, target, lim));

    GroupRun on = runMove(g, start, target, lim, 5'000); // the line goes on behind the targets
    CHECK(on.arrivals == 0);
    CHECK(g.position(0) == 45'000);
    CHECK(g.lead().speed() == approx(3'000).epsilon(0.0001));
    checkOnLine(on);

    g.stop();
    GroupRun end = runMove(g, start, target, lim);
    CHECK(end.finished);
    checkOnLine(end);
}

TEST_CASE("group target with a speed: wrong arguments")
{
    Group g = makeGroup({{0, fast}, {0, fast}});
    int32_t target[] = {1'000, 500};
    CHECK(g.moveAbs(target, -100) == errorCode::invalidArgument);
    CHECK(g.moveAbs(target, 100, -5) == errorCode::invalidArgument);
    CHECK_FALSE(g.isMoving());
    CHECK(g.moveAbs(target, 50'000) == errorCode::speedLimited); // above what the members allow
    GroupRun r = runMove(g, {0, 0}, {1'000, 500}, {fast, fast}, 20'000'000, -1, true);
    CHECK(positions(g) == std::vector<int32_t>{1'000, 500});
}

TEST_CASE("group target with a speed: a way that is too short for the speed is taken without a run-up")
{
    std::vector<MotorLimits> lim{fast, fast};

    SUBCASE("too short to accelerate: the group arrives slower")
    {
        std::vector<int32_t> start{0, 0}, target{20, 10};
        Group g = makeGroup({{0, fast}, {0, fast}});
        CHECK(g.moveAbs(target.data(), 10'000) == errorCode::targetSpeedNotReached);
        GroupRun r = runMove(g, start, target, lim, 20'000'000, -1, true);
        CHECK(r.lead.reversals() == 0);
        CHECK(r.lead.steps() == 20);
        CHECK(positions(g) == target);
        CHECK(speedAtArrival(r) == approx(std::sqrt(2.0 * 50'000 * 20)).epsilon(0.02)); // as fast as 20 steps allow
        checkOnLine(r);
        checkKinematics(r.lead, pathLimits(start, target, lim));

        GroupRun on = runMove(g, start, target, lim, 3'000); // behind the target it goes on to the speed
        CHECK(g.lead().speed() == approx(10'000).epsilon(0.0001));
        checkOnLine(on);
    }
    SUBCASE("too short to decelerate: the group arrives faster")
    {
        std::vector<int32_t> a{0, 0}, b{30'000, 10'000}, c{30'100, 10'050};
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.moveAbs(b.data(), 20'000);
        runMove(g, a, b, lim, 20'000'000, -1, true);
        CHECK(g.moveAbs(c.data(), 1'000) == errorCode::targetSpeedNotReached);
        GroupRun r = runMove(g, b, c, lim, 20'000'000, -1, true);
        CHECK(r.lead.reversals() == 0);
        CHECK(r.lead.steps() == 100);
        CHECK(positions(g) == c);
        CHECK(speedAtArrival(r) == approx(std::sqrt(20'000.0 * 20'000 - 2.0 * 50'000 * 100)).epsilon(0.001));
        checkOnLine(r);

        GroupRun on = runMove(g, b, c, lim, 10'000); // and slows down to the speed behind the target
        CHECK(g.lead().speed() == approx(1'000).epsilon(0.0001));
    }
    SUBCASE("too short to stop: the group passes the target, stops and returns along the line")
    {
        std::vector<int32_t> a{0, 0}, b{30'000, 10'000}, c{30'100, 10'050};
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.moveAbs(b.data(), 20'000);
        runMove(g, a, b, lim, 20'000'000, -1, true);
        CHECK(g.moveAbs(c.data()) == errorCode::targetSpeedNotReached);
        GroupRun r = runMove(g, b, c, lim);
        CHECK(r.finished);
        CHECK(r.lead.reversals() == 1);
        CHECK(positions(g) == c);
        checkOnLine(r);
    }
}

// ------------------------------------------------------------------------------------------- segments

TEST_CASE("group segments: the next move starts at the arrival, without a gap and without a lost step")
{
    std::vector<std::vector<int32_t>> points{{0, 0}, {10'000, 0}, {12'000, 2'000}, {12'000, 10'000}, {0, 3'000}, {0, 0}};
    std::vector<float> speeds{0, 3'000, 3'000, 2'000, 1'000, 0}; // at the points
    std::vector<MotorLimits> lim{fast, fast};
    Group g = makeGroup({{0, fast}, {0, fast}});

    for (size_t i = 1; i < points.size(); i++)
    {
        CAPTURE(i);
        CHECK(g.moveAbs(points[i].data(), speeds[i]) == errorCode::OK);
        CHECK(g.lead().speed() == approx(speeds[i - 1]).epsilon(0.0001)); // the speed of the group is kept
        GroupRun r = runMove(g, points[i - 1], points[i], lim, 20'000'000, -1, speeds[i] != 0);
        CHECK(r.arrivals == 1);
        CHECK(positions(g) == points[i]);
        CHECK(r.lead.reversals() == 0);
        checkOnLine(r);
        if (speeds[i] != 0) CHECK(speedAtArrival(r) == approx(speeds[i]).epsilon(0.0001));
        if (speeds[i - 1] != 0) CHECK(r.lead.s.front().dt <= 1.02 / speeds[i - 1]); // the first step follows as within a move
        if (speeds[i] == 0) CHECK(r.finished);
    }
    CHECK_FALSE(g.isMoving());
}

TEST_CASE("group segments: the speed at which a segment starts can be given")
{
    std::vector<MotorLimits> lim{fast, fast};
    std::vector<int32_t> a{0, 0}, b{10'000, 0}, c{10'000, 8'000};
    Group g = makeGroup({{0, fast}, {0, fast}});

    SUBCASE("at a corner")
    {
        g.moveAbs(b.data(), 3'000);
        runMove(g, a, b, lim, 20'000'000, -1, true);
        CHECK(g.speed(0) == approx(3'000).epsilon(0.0001));
        CHECK(g.moveAbs(c.data(), 0, 1'200) == errorCode::OK);
        CHECK(g.speed(0) == 0.0f);
        CHECK(g.speed(1) == approx(1'200).epsilon(0.0001));
        GroupRun r = runMove(g, b, c, lim);
        CHECK(r.lead.s.front().dt == approx(1.0 / 1'200).epsilon(0.05));
        CHECK(r.finished);
        CHECK(positions(g) == c);
        checkOnLine(r);
    }
    SUBCASE("from standstill")
    {
        CHECK(g.moveAbs(b.data(), 0, 2'000) == errorCode::OK);
        GroupRun r = runMove(g, a, b, lim);
        CHECK(r.lead.s.front().dt == approx(1.0 / 2'000).epsilon(0.05));
        CHECK(positions(g) == b);
    }
    SUBCASE("speed 0 at a corner: the segment starts from standstill")
    {
        g.moveAbs(b.data(), 3'000);
        runMove(g, a, b, lim, 20'000'000, -1, true);
        CHECK(g.moveAbs(c.data(), 0, 0) == errorCode::OK);
        CHECK(g.lead().speed() == 0.0f);
        GroupRun r = runMove(g, b, c, lim);
        CHECK(positions(g) == c);
        checkKinematics(r.lead, pathLimits(b, c, lim));
    }
}

TEST_CASE("group segments: new targets are accepted at standstill and behind the target, not within a segment")
{
    std::vector<MotorLimits> lim{fast, fast};
    std::vector<int32_t> a{0, 0}, b{40'000, 10'000}, c{50'000, 0};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.moveAbs(b.data(), 2'000);
    runMove(g, a, b, lim, 10'000);

    CHECK(g.moveAbs(c.data(), 500) == errorCode::notSupported); // refused: nothing changes
    GroupRun r = runMove(g, a, b, lim, 20'000'000, -1, true);
    CHECK(positions(g) == b);
    CHECK(speedAtArrival(r) == approx(2'000).epsilon(0.0001));
    checkOnLine(r);

    runMove(g, a, b, lim, 300); // some way behind the target
    std::vector<int32_t> here = positions(g);
    CHECK(here[0] == 40'300);
    CHECK(g.moveAbs(c.data()) == errorCode::OK);
    GroupRun r2 = runMove(g, here, c, lim);
    CHECK(r2.finished);
    CHECK(positions(g) == c);
    checkOnLine(r2);

    float speeds[] = {100, 100};
    g.moveAbs(a.data(), 1'000);
    runMove(g, c, a, lim, 20'000'000, -1, true);
    CHECK(g.rotate(speeds) == errorCode::notSupported); // a group that runs on is not turned into a rotation
    g.emergencyStop();
}

TEST_CASE("group segments: the speed at the target of the running segment can be changed")
{
    std::vector<MotorLimits> lim{fast, fast};
    std::vector<int32_t> a{0, 0}, b{40'000, 10'000};

    SUBCASE("to 0: the group stops at the target")
    {
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.moveAbs(b.data(), 5'000);
        runMove(g, a, b, lim, 25'000);
        CHECK(g.moveAbs(b.data(), 0) == errorCode::OK);
        GroupRun r = runMove(g, a, b, lim);
        CHECK(r.finished);
        CHECK(r.lead.reversals() == 0);
        CHECK(positions(g) == b);
        checkOnLine(r);
    }
    SUBCASE("from 0 to a speed")
    {
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.moveAbs(b.data());
        runMove(g, a, b, lim, 25'000);
        CHECK(g.moveAbs(b.data(), 8'000) == errorCode::OK);
        GroupRun r = runMove(g, a, b, lim, 20'000'000, -1, true);
        CHECK(positions(g) == b);
        CHECK(speedAtArrival(r) == approx(8'000).epsilon(0.0001));
        checkOnLine(r);
    }
    SUBCASE("too late to reach it: reported")
    {
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.moveAbs(b.data(), 20'000);
        runMove(g, a, b, lim, 39'990);
        CHECK(g.moveAbs(b.data(), 0) == errorCode::targetSpeedNotReached);
        GroupRun r = runMove(g, a, b, lim);
        CHECK(r.finished);
        CHECK(positions(g) == b);
        checkOnLine(r);
    }
    SUBCASE("after a stop within the segment, the same targets continue it")
    {
        Group g = makeGroup({{0, fast}, {0, fast}});
        g.moveAbs(b.data(), 5'000);
        runMove(g, a, b, lim, 10'000);
        g.stop();
        GroupRun s = runMove(g, a, b, lim);
        CHECK(s.finished);
        CHECK(g.position(0) < 40'000);
        CHECK(g.moveAbs(b.data(), 5'000) == errorCode::OK);
        GroupRun r = runMove(g, positions(g), b, lim, 20'000'000, -1, true);
        CHECK(positions(g) == b);
        CHECK(speedAtArrival(r) == approx(5'000).epsilon(0.0001));
    }
}

TEST_CASE("group segments: a move over zero steps given to a group that runs on stops it at once")
{
    std::vector<MotorLimits> lim{fast, fast};
    std::vector<int32_t> a{0, 0}, b{10'000, 5'000};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.moveAbs(b.data(), 500);
    runMove(g, a, b, lim, 20'000'000, -1, true);
    CHECK(g.moveAbs(b.data()) == errorCode::targetSpeedNotReached);
    GroupEvent e;
    CHECK_FALSE(g.next(e));
    CHECK_FALSE(g.isMoving());
    CHECK(positions(g) == b);
}

// ------------------------------------------------------------------------------------------- random

TEST_CASE("group random: chains of segments reach every point exactly and stay on the lines")
{
    for (unsigned seed = 1; seed <= 150; seed++)
    {
        CAPTURE(seed);
        std::mt19937 gen(seed);
        auto integer = [&gen](int32_t lo, int32_t hi) { return std::uniform_int_distribution<int32_t>(lo, hi)(gen); };
        auto logReal = [&gen](float lo, float hi) { return std::exp(std::uniform_real_distribution<float>(std::log(lo), std::log(hi))(gen)); };

        unsigned n = (unsigned)integer(1, 5);
        std::vector<Member> members;
        std::vector<int32_t> pos;
        std::vector<MotorLimits> lim;
        for (unsigned i = 0; i < n; i++)
        {
            MotorLimits l;
            l.maxSpeed     = logReal(500, 40'000);
            l.acceleration = logReal(2'000, 400'000);
            l.deceleration = integer(0, 1) ? l.acceleration : logReal(2'000, 400'000);
            l.pullInSpeed  = integer(0, 1) ? 0.0f : logReal(1, 500);
            pos.push_back(integer(-10'000, 10'000));
            lim.push_back(l);
            members.push_back({pos[i], l});
        }
        Group g = makeGroup(members);
        if (integer(0, 2) == 0) g.setMaxSpeed(logReal(200, 30'000));
        if (integer(0, 2) == 0) g.setAcceleration(logReal(1'000, 200'000));

        int segments = integer(2, 8);
        for (int k = 0; k < segments; k++)
        {
            CAPTURE(k);
            bool last = k == segments - 1;
            std::vector<int32_t> start = pos, target;
            for (unsigned i = 0; i < n; i++) target.push_back(integer(0, 4) ? pos[i] + integer(-3'000, 3'000) : pos[i]);
            if (target == start) target[0] += 1 + integer(0, 500);
            float exitSpeed = last ? 0.0f : logReal(50, 30'000);
            errorCode code  = integer(0, 3) == 0 ? g.moveAbs(target.data(), exitSpeed, integer(0, 1) ? 0.0f : logReal(50, 5'000))
                                                 : g.moveAbs(target.data(), exitSpeed);
            REQUIRE_FALSE(isError(code));

            if (!last && integer(0, 5) == 0) // change the speed at the target on the way
            {
                GroupRun part = runMove(g, start, target, lim, integer(0, 200), -1, true);
                CHECK(part.worstOff <= 0.5 + 1e-6);
                if (part.arrivals == 0) REQUIRE_FALSE(isError(g.moveAbs(target.data(), logReal(50, 30'000))));
                else
                {
                    pos = target; // already there
                    continue;
                }
            }

            GroupRun r = runMove(g, start, target, lim, 20'000'000, -1, !last);
            CHECK_FALSE(r.lead.runaway);
            CHECK(r.arrivals == 1);
            CHECK_MESSAGE(r.worstOff <= 0.5 + 1e-6, "a member was off the line by ", r.worstOff, " steps");
            CHECK(r.wrongDir == 0);
            CHECK(r.waitSteps == 0);
            if (!last) CHECK(r.lead.reversals() == 0);
            if (last) CHECK(r.finished);
            for (unsigned i = 0; i < n; i++) CHECK(g.position(i) == target[i]);
            pos = target;
        }
        CHECK_FALSE(g.isMoving());
    }
}
