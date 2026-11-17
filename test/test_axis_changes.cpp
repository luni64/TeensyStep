// core::Axis: changes during a move (R11), targets with a speed (R12), stop in every phase (R15).
#include "sim.h"

using namespace TeensyStep;
using namespace sim;

namespace
{
    const Limits L{10'000, 20'000, 20'000, 0};

    Axis makeAxis(const Limits& l = L)
    {
        Axis a;
        a.setMaxSpeed(l.vMax);
        a.setAcceleration(l.acc);
        a.setDeceleration(l.dec);
        a.setPullInSpeed(l.pullIn);
        return a;
    }
}

// ---------------------------------------------------------------------------------- new target (R11)

TEST_CASE("change: a new target further ahead is reached without stopping in between")
{
    Axis a = makeAxis();
    a.moveAbs(20'000);
    run(a, 10'000);
    a.moveAbs(60'000);
    Run r = run(a);
    CHECK(a.position() == 60'000);
    CHECK(r.reversals() == 0);
    for (long i = 0; i < r.steps() - 1; i++) REQUIRE(r.s[i].v > 0); // never at standstill before the end
    checkKinematics(r, L);
}

TEST_CASE("change: a new target behind the motor: decelerate, reverse once, run back")
{
    Axis a = makeAxis();
    a.moveAbs(50'000);
    run(a, 10'000); // at full speed
    a.moveAbs(0);
    Run r = run(a);
    CHECK(a.position() == 0);
    CHECK(r.reversals() == 1);
    checkKinematics(r, L);
}

TEST_CASE("change: a new target too close to stop at: overshoot and return, one reversal")
{
    Axis a = makeAxis();
    a.moveAbs(50'000);
    run(a, 10'000);                 // at 10000 steps/s, braking needs 2500 steps
    a.moveAbs(a.position() + 100); // only 100 steps ahead
    Run r = run(a);
    CHECK(a.position() == 10'100);
    CHECK(r.reversals() == 1);
    int32_t maxX = 0;
    for (auto& e : r.s) maxX = std::max(maxX, e.x);
    CHECK(maxX == approx(12'500).epsilon(0.01)); // overshoots by the braking distance
    checkKinematics(r, L);
}

TEST_CASE("change: the same target again changes nothing")
{
    Axis a = makeAxis();
    a.moveAbs(5'000);
    run(a, 2'000);
    a.moveAbs(5'000);
    Run r = run(a);
    CHECK(a.position() == 5'000);
    CHECK(r.steps() == 3'000);
    CHECK(r.reversals() == 0);
}

// ---------------------------------------------------------------------------------- speed and acceleration (R11)

TEST_CASE("change: a lower maximum speed during a move takes effect at once")
{
    Axis a = makeAxis();
    a.moveAbs(100'000);
    run(a, 10'000);
    a.setMaxSpeed(2'000);
    Run r = run(a, 5'000); // decelerating from 10000 to 2000 takes 2400 steps
    CHECK(a.speed() == approx(2'000).epsilon(0.0001));
    checkKinematics(r, {10'000, 20'000, 20'000, 0});
    Run rest = run(a);
    CHECK(a.position() == 100'000);
    checkKinematics(rest, {2'000, 20'000, 20'000, 0});
}

TEST_CASE("change: a higher maximum speed during a move takes effect at once")
{
    Axis a = makeAxis({2'000, 20'000, 20'000, 0});
    a.moveAbs(100'000);
    run(a, 5'000);
    CHECK(a.speed() == approx(2'000).epsilon(0.0001));
    a.setMaxSpeed(10'000);
    run(a, 10'000);
    CHECK(a.speed() == approx(10'000).epsilon(0.0001));
    run(a);
    CHECK(a.position() == 100'000);
}

TEST_CASE("change: a lower deceleration during a move still stops at the target, by overshooting if needed")
{
    Axis a = makeAxis();
    a.moveAbs(20'000);
    run(a, 18'000);            // 2000 steps left, already braking with 20000 steps/s²
    a.setDeceleration(5'000); // now too close to stop in time
    Run r = run(a);
    CHECK(a.position() == 20'000);
    CHECK(r.reversals() == 1);
    checkKinematics(r, {10'000, 20'000, 5'000, 0});
}

TEST_CASE("change: rotation speed changes, including the direction")
{
    Axis a = makeAxis();
    a.rotate(8'000);
    run(a, 5'000);
    a.rotate(3'000);
    Run r1 = run(a, 5'000);
    CHECK(a.speed() == approx(3'000).epsilon(0.0001));
    CHECK(r1.reversals() == 0);

    a.rotate(-6'000);
    Run r2 = run(a, 5'000);
    CHECK(a.speed() == approx(-6'000).epsilon(0.0001));
    CHECK(r2.reversals() == 1);
    checkKinematics(r1, L);
    checkKinematics(r2, L);
}

// ---------------------------------------------------------------------------------- target with speed (R12)

TEST_CASE("target speed: the target is passed at the given speed, and the motor keeps that speed")
{
    Axis a = makeAxis();
    CHECK(a.moveAbs(30'000, 2'000) == errorCode::OK);
    Run r = run(a, 20'000'000, Until::arrived);
    REQUIRE(r.arrival() >= 0);
    CHECK(a.position() == 30'000);
    CHECK(a.speed() == approx(2'000).epsilon(0.0001));
    CHECK(r.reversals() == 0);
    checkKinematics(r, L);

    Run after = run(a, 1'000); // keeps running at 2000 steps/s
    CHECK_FALSE(after.finished);
    CHECK(a.position() == 31'000);
    for (auto& e : after.s) REQUIRE(e.dt == approx(1.0 / 2'000).epsilon(0.0001));
}

TEST_CASE("target speed: a chain of targets is passed without stopping")
{
    struct Waypoint
    {
        int32_t x;
        float v;
    };
    Axis a = makeAxis();
    Run all;
    for (Waypoint w : {Waypoint{10'000, 2'000}, Waypoint{40'000, 9'000}, Waypoint{50'000, 0}})
    {
        a.moveAbs(w.x, w.v);
        Run r = run(a, 20'000'000, w.v == 0 ? Until::finished : Until::arrived);
        CHECK(a.position() == w.x);
        CHECK(a.speed() == approx(w.v).epsilon(0.0001));
        CHECK(r.reversals() == 0);
        for (long i = 0; i < r.steps() - 1; i++) REQUIRE(r.s[i].v > 0);
        checkKinematics(r, L);
    }
}

TEST_CASE("target speed: too fast and too close needs a run-up with two reversals")
{
    Axis a = makeAxis();
    a.rotate(10'000);
    run(a, 10'000);
    int32_t target = a.position() + 10;
    a.moveAbs(target, 1'000); // cannot slow down to 1000 within 10 steps
    Run r = run(a, 20'000'000, Until::arrived);
    REQUIRE(r.arrival() >= 0);
    CHECK(a.position() == target);
    CHECK(a.speed() == approx(1'000).epsilon(0.0001));
    CHECK(r.reversals() == 2);
    checkKinematics(r, L);
}

TEST_CASE("target speed: too slow and too close needs a run-up with two reversals")
{
    Axis a = makeAxis();
    a.rotate(500);
    run(a, 1'000);
    int32_t target = a.position() + 10;
    a.moveAbs(target, 8'000); // cannot speed up to 8000 within 10 steps
    Run r = run(a, 20'000'000, Until::arrived);
    REQUIRE(r.arrival() >= 0);
    CHECK(a.position() == target);
    CHECK(a.speed() == approx(8'000).epsilon(0.0001));
    CHECK(r.reversals() == 2);
    checkKinematics(r, L);
}

TEST_CASE("target speed: from standstill and too close: back up first, one reversal")
{
    Axis a = makeAxis();
    a.moveAbs(10, 8'000); // 8000 steps/s needs a run-up of 1600 steps
    Run r = run(a, 20'000'000, Until::arrived);
    REQUIRE(r.arrival() >= 0);
    CHECK(a.position() == 10);
    CHECK(a.speed() == approx(8'000).epsilon(0.0001));
    CHECK(r.reversals() == 1);
    int32_t minX = 0;
    for (auto& e : r.s) minX = std::min(minX, e.x);
    CHECK(minX == approx(-1590).epsilon(0.01));
    checkKinematics(r, L);
}

TEST_CASE("target speed: the sign of the speed is the direction in which the target is passed")
{
    SUBCASE("target behind, passed backwards: one reversal")
    {
        Axis a = makeAxis();
        a.rotate(5'000);
        run(a, 5'000);
        a.moveAbs(0, -3'000);
        Run r = run(a, 20'000'000, Until::arrived);
        CHECK(a.position() == 0);
        CHECK(a.speed() == approx(-3'000).epsilon(0.0001));
        CHECK(r.reversals() == 1);
        checkKinematics(r, L);
    }
    SUBCASE("target behind, passed forwards: two reversals")
    {
        Axis a = makeAxis();
        a.rotate(5'000);
        run(a, 5'000);
        a.moveAbs(0, 3'000);
        Run r = run(a, 20'000'000, Until::arrived);
        CHECK(a.position() == 0);
        CHECK(a.speed() == approx(3'000).epsilon(0.0001));
        CHECK(r.reversals() == 2);
        checkKinematics(r, L);
    }
}

TEST_CASE("target speed: with a pull-in speed")
{
    Limits lp{10'000, 20'000, 20'000, 400};
    Axis a = makeAxis(lp);
    a.moveAbs(5'000, 300); // below the pull-in speed
    Run r = run(a, 20'000'000, Until::arrived);
    CHECK(a.position() == 5'000);
    CHECK(a.speed() == approx(300).epsilon(0.0001));
    checkKinematics(r, lp);

    a.moveAbs(5'003, 3'000); // too close: run-up
    Run r2 = run(a, 20'000'000, Until::arrived);
    CHECK(a.position() == 5'003);
    CHECK(a.speed() == approx(3'000).epsilon(0.0001));
    CHECK(r2.reversals() <= 2);
    checkKinematics(r2, lp);
}

// ---------------------------------------------------------------------------------- stop in every phase (R15)

TEST_CASE("stop: in every phase of a move, and a new move directly afterwards")
{
    // 30000 steps: ramp up until 2500, constant until 27500, ramp down
    for (long stopAfter : {0L, 1L, 2L, 100L, 2'499L, 2'500L, 10'000L, 27'500L, 28'000L, 29'998L, 29'999L})
    {
        CAPTURE(stopAfter);
        Axis a = makeAxis();
        a.moveAbs(30'000);
        run(a, stopAfter);
        float v = a.speed();
        int32_t x = a.position();
        a.stop();
        Run r = run(a);
        CHECK(r.finished);
        CHECK(a.speed() == 0.0f);
        CHECK_FALSE(a.isMoving());
        CHECK(r.reversals() == 0);
        CHECK(a.position() - x <= (long)(v * v / (2 * 20'000.0f)) + 2); // not further than the braking distance
        checkKinematics(r, L);

        a.moveAbs(0); // restart at once
        Run back = run(a);
        CHECK(a.position() == 0);
        checkKinematics(back, L);
    }
}

TEST_CASE("stop: with a pull-in speed the motor decelerates to that speed and then stands still")
{
    Limits lp{10'000, 20'000, 20'000, 500};
    Axis a = makeAxis(lp);
    a.rotate(10'000);
    run(a, 10'000);
    a.stop();
    Run r = run(a);
    CHECK(r.finished);
    CHECK(a.speed() == 0.0f);
    CHECK(r.s.back().v > 450); // the last step was made near the pull-in speed, not at a crawl
    CHECK(r.s.back().v <= 500);
    checkKinematics(r, lp);
}

TEST_CASE("stop: on a standing axis does nothing")
{
    Axis a = makeAxis();
    StepEvent e;
    CHECK(a.stop() == errorCode::OK);
    CHECK_FALSE(a.next(e));
    CHECK_FALSE(a.isMoving());
}

TEST_CASE("setPosition: during a move the target stays where it is")
{
    Axis a = makeAxis();
    a.moveAbs(10'000);
    run(a, 4'000);
    a.setPosition(0); // e.g. at an index mark
    Run r = run(a);
    CHECK(a.position() == 10'000);
    CHECK(r.steps() == 10'000);
}
