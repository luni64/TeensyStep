// core::Group: several motors that move synchronized. Requirements R1 (at most half a step off the line, in
// every state), R11 (speed and acceleration can change during a move, the targets cannot), R20 (rotation with
// a fixed speed ratio, speed factor), and bug pattern 9 of user-feedback.md.
#include "groupsim.h"

#include <random>

using namespace TeensyStep;
using namespace TeensyStep::core;
using namespace sim;

using namespace groupsim;

// ------------------------------------------------------------------------------------------- moves

TEST_CASE("group: all members reach their targets with the same event, on a straight line")
{
    std::vector<int32_t> start{0, 100, -50};
    std::vector<int32_t> target{40'000, 100 - 13'337, -50 + 1};
    std::vector<MotorLimits> lim{fast, fast, fast};
    Group g = makeGroup({{start[0], fast}, {start[1], fast}, {start[2], fast}});

    CHECK(g.moveAbs(target.data()) == errorCode::OK);
    CHECK(g.isMoving());
    GroupRun r = runMove(g, start, target, lim);
    CHECK(r.finished);
    CHECK(r.arrivals == 1);
    CHECK(r.lead.s.back().arrived); // the arrival flag comes with the last step
    for (unsigned i = 0; i < 3; i++) CHECK(g.position(i) == target[i]);
    CHECK_FALSE(g.isMoving());
    CHECK(r.lead.steps() == 40'000);
    checkOnLine(r);
    checkKinematics(r.lead, pathLimits(start, target, lim));
}

TEST_CASE("group: a single member behaves like a single motor")
{
    Group g = makeGroup({{0, fast}});
    int32_t target[] = {-30'000};
    g.moveAbs(target);
    GroupRun r = runMove(g, {0}, {-30'000}, {fast});
    CHECK(g.position(0) == -30'000);
    CHECK(r.lead.peakSpeed() == approx(20'000).epsilon(0.0001));
    CHECK(r.lead.duration() == approx(idealDuration(30'000, 20'000, 50'000)).epsilon(0.002));
    checkOnLine(r);
}

TEST_CASE("group: the group is as fast as its members allow (#107)")
{
    SUBCASE("the slow motor has the longest way: it sets the pace")
    {
        std::vector<int32_t> start{0, 0}, target{10'000, 40'000};
        std::vector<MotorLimits> lim{fast, slow};
        Group g = makeGroup({{0, fast}, {0, slow}});
        g.moveAbs(target.data());
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.lead.peakSpeed() == approx(5'000).epsilon(0.0001));
        checkOnLine(r);
        checkKinematics(r.lead, pathLimits(start, target, lim));
    }
    SUBCASE("the slow motor has a short way: the fast one is not held back to the slow speed")
    {
        std::vector<int32_t> start{0, 0}, target{40'000, 4'000}; // ratio 0.1: the slow motor allows 50000 on the path
        std::vector<MotorLimits> lim{fast, slow};
        Group g = makeGroup({{0, fast}, {0, slow}});
        g.moveAbs(target.data());
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.lead.peakSpeed() == approx(20'000).epsilon(0.0001));
        checkOnLine(r);
    }
    SUBCASE("in between: the slow motor limits the path to its speed divided by its share")
    {
        std::vector<int32_t> start{0, 0}, target{40'000, 20'000}; // ratio 0.5: 5000 / 0.5 = 10000 on the path
        std::vector<MotorLimits> lim{fast, slow};
        Group g = makeGroup({{0, fast}, {0, slow}});
        g.moveAbs(target.data());
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.lead.peakSpeed() == approx(10'000).epsilon(0.0001));
        CHECK(std::fabs(g.speed(1)) == 0.0f);
        checkOnLine(r);
        checkKinematics(r.lead, pathLimits(start, target, lim)); // also acceleration 10000 / 0.5 = 20000
    }
}

TEST_CASE("group: members that do not move, and a move over zero steps")
{
    std::vector<int32_t> start{5, 7, 9}, target{5, 7 + 3'000, 9};
    std::vector<MotorLimits> lim{fast, fast, fast};
    Group g = makeGroup({{5, fast}, {7, fast}, {9, fast}});
    g.moveAbs(target.data());
    GroupRun r = runMove(g, start, target, lim);
    CHECK(g.position(0) == 5);
    CHECK(g.position(1) == 3'007);
    CHECK(g.position(2) == 9);
    checkOnLine(r);

    CHECK(g.moveAbs(target.data()) == errorCode::OK); // already there
    GroupEvent e;
    CHECK_FALSE(g.next(e));
    CHECK_FALSE(g.isMoving());
}

TEST_CASE("group: the speed of each member is its share of the path speed")
{
    Group g = makeGroup({{0, fast}, {0, fast}});
    int32_t target[] = {40'000, -10'000};
    g.moveAbs(target);
    runMove(g, {0, 0}, {40'000, -10'000}, {fast, fast}, 20'000);
    CHECK(g.speed(0) == approx(20'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(-5'000).epsilon(0.0001));
}

// ------------------------------------------------------------------------------------------- stop and changes during a move

TEST_CASE("group: a stop during a move keeps all members on the line")
{
    for (long stopAfter : {0L, 1L, 500L, 5'000L, 20'000L, 39'000L, 39'999L})
    {
        CAPTURE(stopAfter);
        std::vector<int32_t> start{0, 0, 0}, target{40'000, -23'456, 77};
        std::vector<MotorLimits> lim{fast, fast, slow};
        Group g = makeGroup({{0, fast}, {0, fast}, {0, slow}});
        g.moveAbs(target.data());
        runMove(g, start, target, lim, stopAfter);
        CHECK(g.stop() == errorCode::OK);
        GroupRun r = runMove(g, start, target, lim);
        CHECK(r.finished);
        CHECK(r.lead.reversals() == 0);
        CHECK_FALSE(g.isMoving());
        checkOnLine(r);

        std::vector<int32_t> here{g.position(0), g.position(1), g.position(2)};
        CHECK(g.moveAbs(target.data()) == errorCode::OK); // continue to the targets from where the group stopped
        GroupRun rest = runMove(g, here, target, lim);
        for (unsigned i = 0; i < 3; i++) CHECK(g.position(i) == target[i]);
        checkOnLine(rest);
    }
}

TEST_CASE("group: emergency stop")
{
    Group g = makeGroup({{0, fast}, {0, fast}});
    int32_t target[] = {40'000, 10'000};
    g.moveAbs(target);
    runMove(g, {0, 0}, {40'000, 10'000}, {fast, fast}, 10'000);
    g.emergencyStop();
    GroupEvent e;
    CHECK_FALSE(g.isMoving());
    CHECK_FALSE(g.next(e));
    CHECK(g.position(0) == 10'000);
    CHECK(g.position(1) == 2'500);
    CHECK(g.speed(0) == 0.0f);
}

TEST_CASE("group: new targets during a move are refused, the move goes on")
{
    std::vector<int32_t> start{0, 0}, target{40'000, 10'000};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.moveAbs(target.data());
    runMove(g, start, target, {fast, fast}, 10'000);
    int32_t other[] = {0, 0};
    CHECK(g.moveAbs(other) == errorCode::notSupported);
    float speeds[] = {100, 100};
    CHECK(g.rotate(speeds) == errorCode::notSupported);
    GroupRun r = runMove(g, start, target, {fast, fast});
    CHECK(g.position(0) == 40'000);
    CHECK(g.position(1) == 10'000);
    checkOnLine(r);
}

TEST_CASE("group: the limits of a member can change during a move")
{
    std::vector<int32_t> start{0, 0}, target{100'000, 50'000};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.moveAbs(target.data());
    runMove(g, start, target, {fast, fast}, 20'000);
    CHECK(g.lead().speed() == approx(20'000).epsilon(0.0001));

    g.setLimits(1, slow); // the follower (share 0.5) now allows 5000: the path slows down to 10000
    GroupRun r = runMove(g, start, target, {fast, fast}, 30'000); // (while it slows down, the old limit still applies)
    CHECK(g.lead().speed() == approx(10'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(5'000).epsilon(0.0001));
    checkOnLine(r);

    GroupRun rest = runMove(g, start, target, {fast, slow});
    CHECK(g.position(0) == 100'000);
    CHECK(g.position(1) == 50'000);
    checkOnLine(rest);
}

TEST_CASE("group: a deceleration lowered late makes the group overshoot and return along the line")
{
    std::vector<int32_t> start{0, 0}, target{40'000, -15'000};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.moveAbs(target.data());
    runMove(g, start, target, {fast, fast}, 38'000); // braking has begun
    MotorLimits weak = fast;
    weak.deceleration = 5'000;
    g.setLimits(0, weak);
    GroupRun r = runMove(g, start, target, {weak, fast});
    CHECK(r.finished);
    CHECK(g.position(0) == 40'000);
    CHECK(g.position(1) == -15'000);
    CHECK(r.lead.reversals() == 1);
    int32_t far = 0;
    for (auto& s : r.lead.s) far = std::max(far, s.x);
    CHECK(far > 40'500); // it really went past the targets
    checkOnLine(r);      // and stayed on the line beyond them
}

TEST_CASE("group: no event is longer than the maximum delay")
{
    MotorLimits crawl{2, 10, 10, 0};
    Group g = makeGroup({{0, crawl}, {0, crawl}});
    g.setMaxDelay(0.0005f);
    int32_t target[] = {6, 3};
    g.moveAbs(target);
    GroupRun r = runMove(g, {0, 0}, {6, 3}, {crawl, crawl});
    CHECK(g.position(0) == 6);
    CHECK(g.position(1) == 3);
    CHECK(r.lead.maxEventDelay <= 0.0005 * 1.0001);
    CHECK(r.lead.waits > 1'000);
    checkOnLine(r);
}

// ------------------------------------------------------------------------------------------- rotation

namespace
{
    GroupRun runRotation(Group& g, const std::vector<int32_t>& start, const std::vector<float>& speeds, const std::vector<MotorLimits>& limits,
                         long maxSteps)
    {
        unsigned leadIdx = 0;
        for (unsigned i = 0; i < speeds.size(); i++)
            if (std::fabs(speeds[i]) > std::fabs(speeds[leadIdx])) leadIdx = i;
        std::vector<double> perStep(speeds.size());
        for (unsigned i = 0; i < speeds.size(); i++) perStep[i] = (double)speeds[i] / std::fabs(speeds[leadIdx]);
        double sign = speeds[leadIdx] > 0 ? 1 : -1;
        auto path = [&g, &start, leadIdx, sign] { return sign * ((double)g.position(leadIdx) - start[leadIdx]); }; // steps of the fastest motor
        return runGroup(g, start, perStep, path, limits, maxSteps);
    }
}

TEST_CASE("group rotation: the speed ratio is kept exactly, also while accelerating")
{
    std::vector<int32_t> start{0, 0, 0};
    std::vector<float> speeds{5'000, 15'000, -1'234.5f};
    std::vector<MotorLimits> lim{fast, fast, fast};
    Group g = makeGroup({{0, fast}, {0, fast}, {0, fast}});
    CHECK(g.rotate(speeds.data()) == errorCode::OK);
    GroupRun r = runRotation(g, start, speeds, lim, 60'000);
    CHECK_FALSE(r.finished);
    CHECK(g.speed(0) == approx(5'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(15'000).epsilon(0.0001));
    CHECK(g.speed(2) == approx(-1'234.5).epsilon(0.0001));
    CHECK(g.position(1) == 60'000);
    CHECK(g.position(0) == 20'000);
    checkOnLine(r);
}

TEST_CASE("group rotation: the speed factor changes all speeds and keeps the ratio; a negative factor reverses all")
{
    std::vector<int32_t> start{0, 0};
    std::vector<float> speeds{4'000, -16'000};
    std::vector<MotorLimits> lim{fast, fast};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.rotate(speeds.data());
    GroupRun r1 = runRotation(g, start, speeds, lim, 20'000);

    CHECK(g.overrideSpeed(0.5f) == errorCode::OK);
    GroupRun r2 = runRotation(g, start, speeds, lim, 20'000);
    CHECK(g.speed(0) == approx(2'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(-8'000).epsilon(0.0001));
    CHECK(r2.lead.reversals() == 0);

    CHECK(g.overrideSpeed(-1.0f) == errorCode::OK);
    GroupRun r3 = runRotation(g, start, speeds, lim, 20'000);
    CHECK(g.speed(0) == approx(-4'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(16'000).epsilon(0.0001));
    CHECK(r3.lead.reversals() == 1);

    checkOnLine(r1);
    checkOnLine(r2);
    checkOnLine(r3); // running backwards, the members retrace the same line
}

TEST_CASE("group rotation: factor 0 stops the group, a later factor starts it again with the same ratio (pattern 13)")
{
    std::vector<int32_t> start{0, 0};
    std::vector<float> speeds{4'000, -16'000};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.rotate(speeds.data());
    runRotation(g, start, speeds, {fast, fast}, 10'000);
    CHECK(g.overrideSpeed(0) == errorCode::OK);
    GroupRun r = runRotation(g, start, speeds, {fast, fast}, 20'000'000);
    CHECK(r.finished);
    CHECK_FALSE(g.isMoving());

    CHECK(g.overrideSpeed(0.25f) == errorCode::OK);
    GroupRun r2 = runRotation(g, start, speeds, {fast, fast}, 10'000);
    CHECK(g.speed(0) == approx(1'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(-4'000).epsilon(0.0001));
    checkOnLine(r);
    checkOnLine(r2);

    Group idle = makeGroup({{0, fast}, {0, fast}});
    CHECK(idle.overrideSpeed(0.5f) == errorCode::notSupported); // nothing to scale: the group has not been rotated
}

TEST_CASE("group rotation: a reversal at any position retraces the line exactly")
{
    MotorLimits jumpy{1'000, 10'000, 10'000, 200}; // speeds below the pull-in speed: the group reverses on the spot
    for (long stepsBefore : {1L, 2L, 3L, 4L, 5L, 6L, 7L, 8L})
    {
        CAPTURE(stepsBefore);
        std::vector<int32_t> start{0, 0, 0};
        std::vector<float> speeds{50, 25, 12.5f}; // shares 1, 1/2 and 1/4: the members step on every 2nd and 4th path step
        Group g = makeGroup({{0, jumpy}, {0, jumpy}, {0, jumpy}});
        g.rotate(speeds.data());
        GroupRun there = runRotation(g, start, speeds, {jumpy, jumpy, jumpy}, stepsBefore);
        g.overrideSpeed(-1.0f);
        GroupRun back = runRotation(g, start, speeds, {jumpy, jumpy, jumpy}, stepsBefore + 9);
        CHECK(back.lead.reversals() == 1);
        CHECK(g.position(0) == -9);
        checkOnLine(there);
        checkOnLine(back);
    }
}

TEST_CASE("group rotation: running backwards, every member passes the same positions as on the way there")
{
    MotorLimits jumpy{1'000, 10'000, 10'000, 200};
    std::vector<float> speeds{50, 25, 12.5f, 33.3f};
    Group g = makeGroup({{0, jumpy}, {0, jumpy}, {0, jumpy}, {0, jumpy}});
    g.rotate(speeds.data());

    std::vector<std::vector<int32_t>> seen(41, std::vector<int32_t>(4)); // positions of the members at each position of the fastest one
    GroupEvent e;
    while (g.position(0) < 40)
    {
        REQUIRE(g.next(e));
        if (e.stepMask != 0)
            for (unsigned i = 0; i < 4; i++) seen[g.position(0)][i] = g.position(i);
    }
    g.overrideSpeed(-1.0f);
    long differences = 0;
    while (g.position(0) > 0)
    {
        REQUIRE(g.next(e));
        if (e.stepMask != 0)
            for (unsigned i = 0; i < 4; i++) differences += g.position(i) != seen[g.position(0)][i];
    }
    CHECK(differences == 0);
    for (unsigned i = 0; i < 4; i++) CHECK(g.position(i) == 0);
}

TEST_CASE("group rotation: stop, and rotation again")
{
    std::vector<int32_t> start{0, 0};
    std::vector<float> speeds{4'000, 16'000};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.rotate(speeds.data());
    runRotation(g, start, speeds, {fast, fast}, 10'000);
    g.stop();
    GroupRun r = runRotation(g, start, speeds, {fast, fast}, 20'000'000);
    CHECK(r.finished);
    CHECK_FALSE(g.isMoving());
    CHECK(g.speed(1) == 0.0f);
    checkOnLine(r);

    std::vector<int32_t> here{g.position(0), g.position(1)};
    std::vector<float> other{-3'000, 1'000}; // a new ratio is allowed once the group stands still
    CHECK(g.rotate(other.data()) == errorCode::OK);
    GroupRun r2 = runRotation(g, here, other, {fast, fast}, 5'000);
    CHECK(g.speed(0) == approx(-3'000).epsilon(0.0001));
    checkOnLine(r2);
}

TEST_CASE("group rotation: a member with speed 0 stands still (pattern 9, #139); all speeds 0 is a stop")
{
    std::vector<int32_t> start{0, 0};
    std::vector<float> speeds{0, 8'000};
    Group g = makeGroup({{0, fast}, {0, fast}});
    CHECK(g.rotate(speeds.data()) == errorCode::OK);
    GroupRun r = runRotation(g, start, speeds, {fast, fast}, 10'000);
    CHECK(g.position(0) == 0);
    CHECK(g.position(1) == 10'000);
    checkOnLine(r);

    float zero[] = {0, 0};
    CHECK(g.rotate(zero) == errorCode::OK);
    GroupRun r2 = runRotation(g, start, speeds, {fast, fast}, 20'000'000);
    CHECK(r2.finished);
    CHECK_FALSE(g.isMoving());
}

TEST_CASE("group rotation: a new ratio while rotating is refused; the same ratio changes the speed")
{
    std::vector<int32_t> start{0, 0};
    std::vector<float> speeds{4'000, 16'000};
    Group g = makeGroup({{0, fast}, {0, fast}});
    g.rotate(speeds.data());
    runRotation(g, start, speeds, {fast, fast}, 20'000);

    float other[] = {4'000, 8'000};
    CHECK(g.rotate(other) == errorCode::notSupported);
    float oneReversed[] = {-4'000, 16'000}; // same amounts, but only one member changes its direction
    CHECK(g.rotate(oneReversed) == errorCode::notSupported);
    int32_t target[] = {0, 0};
    CHECK(g.moveAbs(target) == errorCode::notSupported);

    float half[] = {2'000, 8'000};
    CHECK(g.rotate(half) == errorCode::OK);
    GroupRun r = runRotation(g, start, speeds, {fast, fast}, 20'000);
    CHECK(g.speed(1) == approx(8'000).epsilon(0.0001));
    checkOnLine(r);
}

TEST_CASE("group rotation: speeds above a member's maximum are limited as a group and reported")
{
    std::vector<int32_t> start{0, 0};
    std::vector<float> speeds{10'000, 20'000}; // the slow motor allows only 5000
    Group g = makeGroup({{0, slow}, {0, fast}});
    CHECK(g.rotate(speeds.data()) == errorCode::speedLimited);
    GroupRun r = runRotation(g, start, speeds, {slow, fast}, 40'000);
    CHECK(g.speed(0) == approx(5'000).epsilon(0.0001));
    CHECK(g.speed(1) == approx(10'000).epsilon(0.0001)); // the ratio is kept
    checkOnLine(r);
}

// ------------------------------------------------------------------------------------------- members

TEST_CASE("group: number of members")
{
    Group g;
    CHECK(g.setMembers(0) == errorCode::invalidArgument);
    CHECK(g.setMembers(Group::maxMembers + 1) == errorCode::invalidArgument);
    CHECK(g.setMembers(Group::maxMembers) == errorCode::OK);

    std::vector<int32_t> start(Group::maxMembers, 0), target;
    std::vector<MotorLimits> lim(Group::maxMembers, fast);
    for (unsigned i = 0; i < Group::maxMembers; i++)
    {
        g.setLimits(i, fast);
        target.push_back((int32_t)(1'000 * (i + 1)) * (i % 2 ? -1 : 1));
    }
    g.moveAbs(target.data());
    GroupRun r = runMove(g, start, target, lim);
    for (unsigned i = 0; i < Group::maxMembers; i++) CHECK(g.position(i) == target[i]);
    checkOnLine(r);
}

// ------------------------------------------------------------------------------------------- random

TEST_CASE("group random: moves with stops and limit changes stay on the line and end at the targets")
{
    for (unsigned seed = 1; seed <= 150; seed++)
    {
        CAPTURE(seed);
        std::mt19937 gen(seed);
        auto integer = [&gen](int32_t lo, int32_t hi) { return std::uniform_int_distribution<int32_t>(lo, hi)(gen); };
        auto logReal = [&gen](float lo, float hi) { return std::exp(std::uniform_real_distribution<float>(std::log(lo), std::log(hi))(gen)); };
        auto randomLimits = [&] {
            MotorLimits l;
            l.maxSpeed     = logReal(100, 40'000);
            l.acceleration = logReal(500, 400'000);
            l.deceleration = integer(0, 1) ? l.acceleration : logReal(500, 400'000);
            l.pullInSpeed  = integer(0, 1) ? 0.0f : logReal(1, 500);
            return l;
        };

        unsigned n = (unsigned)integer(1, 6);
        std::vector<Member> members;
        std::vector<int32_t> pos;
        std::vector<MotorLimits> lim;
        for (unsigned i = 0; i < n; i++)
        {
            pos.push_back(integer(-10'000, 10'000));
            lim.push_back(randomLimits());
            members.push_back({pos[i], lim[i]});
        }
        Group g = makeGroup(members);

        for (int move = 0; move < 4; move++)
        {
            CAPTURE(move);
            std::vector<int32_t> start = pos, target;
            for (unsigned i = 0; i < n; i++) target.push_back(integer(0, 3) ? pos[i] + integer(-60'000, 60'000) : pos[i]);
            REQUIRE(g.moveAbs(target.data()) == errorCode::OK);

            int action = integer(0, 2); // 0: run through, 1: change limits on the way, 2: stop on the way
            std::vector<MotorLimits> allowed = lim; // maximum speeds that apply to the rest of the move
            if (action != 0)
            {
                GroupRun part = runMove(g, start, target, lim, integer(0, 30'000), integer(0, 1) ? -1 : 0.3);
                checkOnLine(part);
                if (action == 1)
                {
                    unsigned i = (unsigned)integer(0, (int32_t)n - 1);
                    lim[i]     = randomLimits();
                    g.setLimits(i, lim[i]);
                    allowed[i].maxSpeed = std::max(allowed[i].maxSpeed, lim[i].maxSpeed); // the old one applies while the group slows down
                }
                else
                {
                    g.stop();
                }
            }
            GroupRun r = runMove(g, start, target, allowed);
            REQUIRE(r.finished);
            checkOnLine(r);
            CHECK_FALSE(r.lead.runaway);
            if (action == 0 && r.lead.steps() > 0) checkKinematics(r.lead, pathLimits(start, target, lim));
            for (unsigned i = 0; i < n; i++) pos[i] = g.position(i);
            if (action != 2)
                for (unsigned i = 0; i < n; i++) CHECK(pos[i] == target[i]);
        }
    }
}
