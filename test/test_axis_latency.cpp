// core::Axis: no event waits longer than the maximum delay, and a command takes effect within it, also in the
// middle of a long step period (R11).
#include "sim.h"

using namespace TeensyStep;
using namespace sim;

namespace
{
    Axis makeAxis(float vMax, float acc, float pullIn = 0)
    {
        Axis a;
        a.setMaxSpeed(vMax);
        a.setAcceleration(acc);
        a.setPullInSpeed(pullIn);
        return a;
    }

    // an axis that turns steadily at 0.5 steps/s (one step every 2 s) and is 0.1 s into a step period
    Axis slowAxis()
    {
        Axis a = makeAxis(1'000, 20'000);
        a.rotate(0.5f);
        run(a, 3);
        Run r = runFor(a, 0.1);
        REQUIRE(r.steps() == 0);
        REQUIRE(r.endedInWait);
        return a;
    }
}

TEST_CASE("delay: the default maximum delay of an event is 1 ms")
{
    Axis a = makeAxis(1'000, 20'000);
    a.rotate(0.5f);
    Run r = run(a, 3);
    CHECK(r.maxEventDelay <= 0.001 * 1.0001);
    CHECK(r.waits > 3'000);
}

TEST_CASE("delay: with a maximum delay the steps come at the same times as without")
{
    Run runs[3];
    float maxDelay[3] = {Axis::noDelayLimit, 0.001f, 0.000437f};
    for (int i = 0; i < 3; i++)
    {
        Axis a = makeAxis(2.0f, 10);
        a.setMaxDelay(maxDelay[i]);
        a.moveAbs(6);
        runs[i] = run(a);
        CHECK(runs[i].finished);
        CHECK(a.position() == 6);
        REQUIRE(runs[i].steps() == 6);
    }
    CHECK(runs[0].waits == 0);
    CHECK(runs[1].waits > 2'000);
    CHECK(runs[1].maxEventDelay <= 0.001 * 1.0001);
    CHECK(runs[2].maxEventDelay <= 0.000437 * 1.0001);
    for (int i = 0; i < 6; i++)
    {
        CHECK(runs[1].s[i].t == approx(runs[0].s[i].t).epsilon(1e-4));
        CHECK(runs[2].s[i].t == approx(runs[0].s[i].t).epsilon(1e-4));
    }
}

TEST_CASE("delay: very long step periods are still exact")
{
    Axis a = makeAxis(0.002f, 10); // one step every 500 s
    a.rotate(0.002f);
    Run r = run(a, 3);
    CHECK(r.s[2].dt == approx(500.0).epsilon(1e-4));
    CHECK(r.maxEventDelay <= 0.001 * 1.0001);
}

TEST_CASE("delay: a move at a very low maximum speed keeps that speed exactly")
{
    Axis a = makeAxis(0.002f, 10);
    a.moveAbs(5);
    Run r = run(a);
    CHECK(a.position() == 5);
    CHECK(r.s[2].dt == approx(500.0).epsilon(1e-4));
}

TEST_CASE("delay: at high speed there are no wait events")
{
    Axis a = makeAxis(10'000, 20'000);
    a.rotate(5'000);
    run(a, 2'000);
    Run r = run(a, 1'000);
    CHECK(r.waits == 0);
}

TEST_CASE("delay: a new speed during a long step period takes effect at once")
{
    Axis a = slowAxis(); // 1.9 s until the next step
    a.rotate(1'000);
    Run r = run(a, 1, Until::finished, -1, true);
    CHECK(r.time < 0.02); // the rest of the step is made at the new, rising speed
    run(a, 2'000);
    CHECK(a.speed() == approx(1'000).epsilon(0.0001));
}

TEST_CASE("delay: a stop during a long step period makes no further step")
{
    Axis a = slowAxis();
    int32_t x = a.position();
    a.stop();
    Run r = run(a);
    CHECK(r.finished);
    CHECK(r.steps() == 0);
    CHECK(r.time <= 0.001 * 1.0001);
    CHECK(a.position() == x);
    CHECK(a.speed() == 0.0f);
    CHECK_FALSE(a.isMoving());
}

TEST_CASE("delay: a new target at the current position during a long step period makes no further step")
{
    Axis a = makeAxis(1.0f, 20'000);
    a.moveAbs(1'000);
    run(a, 3);
    Run w = runFor(a, 0.3);
    REQUIRE(w.steps() == 0);
    a.moveAbs(a.position());
    Run r = run(a);
    CHECK(r.finished);
    CHECK(r.steps() == 0);
    CHECK(a.position() == 3);
}

TEST_CASE("delay: a reversal during a long step period")
{
    Axis a = slowAxis();
    int32_t x = a.position();
    a.rotate(-0.5f);
    Run r = run(a, 1, Until::finished, -1, true);
    REQUIRE(r.steps() == 1);
    CHECK(r.s[0].dir == -1);
    CHECK(a.position() == x - 1);
    CHECK(r.time == approx(2.0).epsilon(0.01)); // a full period at the new speed, from standstill
}

TEST_CASE("delay: a change that does not alter the step leaves its time unchanged")
{
    Axis a = slowAxis();   // 0.1 s into the period
    a.setMaxSpeed(1'000); // same value as before
    Run r = run(a, 1, Until::finished, -1, true);
    CHECK(0.1 + r.time == approx(2.0).epsilon(0.001));
}

TEST_CASE("delay: the first step at a low speed comes after one period, not after a long ramp")
{
    Axis a = makeAxis(1'000, 20'000);
    a.rotate(0.5f); // reached almost at once with 20000 steps/s²
    Run r = run(a, 2);
    CHECK(r.s[0].dt == approx(2.0).epsilon(0.01));
    CHECK(r.s[1].dt == approx(2.0).epsilon(0.001));
}

TEST_CASE("delay: the last step before a stop at a low speed takes one period")
{
    Axis a = makeAxis(10, 500'000); // stops within a fraction of a step
    a.moveAbs(5);
    Run r = run(a);
    CHECK(a.position() == 5);
    CHECK(r.s[4].dt == approx(0.1).epsilon(0.01));
    CHECK(r.duration() == approx(0.5).epsilon(0.01));
}
