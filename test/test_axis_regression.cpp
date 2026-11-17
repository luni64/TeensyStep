// core::Axis: bug patterns of version 2 (user-feedback.md, section 6). The numbers in the test names are the
// numbers of the patterns there; #n are the GitHub issues.
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

    // a move from standstill to standstill: exact end position, limits kept, speed rises and falls only once
    void checkMove(float vMax, float acc, int32_t dist, float pullIn = 0)
    {
        CAPTURE(vMax);
        CAPTURE(acc);
        CAPTURE(dist);
        Axis a = makeAxis(vMax, acc, pullIn);
        a.moveAbs(dist);
        Run r = run(a);
        CHECK(r.finished);
        CHECK(a.position() == dist);
        CHECK(a.speed() == 0.0f);
        checkKinematics(r, {vMax, acc, acc, pullIn});

        long falls = 0, risesAfterFall = 0;
        for (long i = 1; i < r.steps() - 1; i++)
        {
            if (r.s[i].v < r.s[i - 1].v) falls++;
            if (falls > 0 && r.s[i].v > r.s[i - 1].v) risesAfterFall++;
        }
        CHECK_MESSAGE(risesAfterFall == 0, "the speed rises again after the deceleration has started");

        double sRamp = (double)vMax * vMax / (2.0 * acc);
        if (dist > 2 * sRamp + 10) CHECK(r.peakSpeed() == approx(vMax).epsilon(0.0005));
    }
}

TEST_CASE("pattern 1 (#118): maximum speeds around 65536 have a normal ramp")
{
    for (float vMax : {65'535.0f, 65'536.0f, 66'000.0f, 70'000.0f})
        for (float acc : {60'000.0f, 100'000.0f}) checkMove(vMax, acc, 200'000);
}

TEST_CASE("pattern 2 (#54): 300000 steps/s with 500000 steps/s² ramps up without dropping back")
{
    checkMove(300'000, 500'000, 1'000'000);
}

TEST_CASE("pattern 3 (#66): 70000 steps/s with 20000 steps/s²")
{
    checkMove(70'000, 20'000, 600'000);
}

TEST_CASE("pattern 4 (#70, #71): no step-rate jump for particular distances")
{
    for (int32_t d : {227, 347, 1243, 2497, 4521}) checkMove(20'800, 10'400, d);
    for (int32_t d : {83, 389, 415, 613}) checkMove(800, 400, d);
    for (int32_t d : {95, 187, 243, 311}) checkMove(65'000, 180'000, d);
}

TEST_CASE("pattern 5 (#61): high acceleration with low speed stops at the target")
{
    for (float vMax : {10.0f, 100.0f, 1'000.0f})
        for (int32_t d : {1, 5, 100, 10'000}) checkMove(vMax, 500'000, d);
}

TEST_CASE("pattern 6 (#16): pull-in speed of 40 % of the maximum speed")
{
    checkMove(10'000, 20'000, 20'000, 4'000);
    checkMove(1'000, 2'000, 300, 400);
}

TEST_CASE("pattern 7 (#13): the same target again makes no step; targets at the ends of the range work")
{
    Axis a = makeAxis(10'000, 100'000);
    a.moveAbs(1'000);
    run(a);
    a.moveAbs(1'000);
    Run r = run(a);
    CHECK(r.steps() == 0);

    Axis b = makeAxis(100'000, 1'000'000);
    b.setPosition(INT32_MAX - 5'000);
    b.moveAbs(INT32_MAX);
    run(b);
    CHECK(b.position() == INT32_MAX);

    Axis c = makeAxis(100'000, 1'000'000);
    c.setPosition(INT32_MIN + 5'000);
    c.moveAbs(INT32_MIN);
    run(c);
    CHECK(c.position() == INT32_MIN);
}

TEST_CASE("pattern 8 (#106): an acceleration above the supported range is reported")
{
    Axis a;
    CHECK(a.setAcceleration(1e10f) == errorCode::accelerationLimited);
    CHECK(a.setAcceleration(500'000) == errorCode::OK);
    CHECK(a.setAcceleration(5'000'000) == errorCode::OK); // above the limit of version 2
}

TEST_CASE("pattern 10 (#92): a move over zero steps followed by stop does not hang")
{
    Axis a = makeAxis(10'000, 20'000);
    a.moveAbs(0);
    a.stop();
    StepEvent e;
    CHECK_FALSE(a.next(e));
    CHECK_FALSE(a.isMoving());
    CHECK(a.speed() == 0.0f);

    a.moveAbs(500); // stop directly after the start
    a.stop();
    Run r = run(a);
    CHECK(r.finished);
    CHECK(r.steps() == 0);
    CHECK_FALSE(a.isMoving());

    a.moveAbs(500); // stop at the instant the target is reached
    run(a, 499);
    a.stop();
    run(a);
    CHECK_FALSE(a.isMoving());
    CHECK(a.speed() == 0.0f);
}

TEST_CASE("pattern 12 (#47): rotate, stop, rotate again, many times")
{
    Axis a = makeAxis(5'000, 50'000);
    for (int i = 0; i < 20; i++)
    {
        a.rotate(5'000);
        run(a, 1'000);
        CHECK(a.speed() == approx(5'000).epsilon(0.0001));
        a.stop();
        Run r = run(a);
        CHECK(r.finished);
        CHECK(a.speed() == 0.0f);
    }
}

TEST_CASE("pattern 13 (#119): speed 0 and then a speed again restarts the motor")
{
    Axis a = makeAxis(5'000, 50'000);
    a.rotate(5'000);
    run(a, 1'000);
    a.rotate(0);
    run(a);
    CHECK_FALSE(a.isMoving());
    a.rotate(3'000);
    run(a, 1'000);
    CHECK(a.speed() == approx(3'000).epsilon(0.0001));
}

TEST_CASE("pattern 14 (#150, #111, #115): reversing the rotation never freezes")
{
    struct P
    {
        float vMax, acc, pullIn, v;
    };
    for (P p : {P{300'000, 200'000, 0, 100'000}, P{300'000, 250'000, 0, 30'000}, P{300'000, 5'000'000, 5'000, 100'000}})
    {
        CAPTURE(p.acc);
        Axis a = makeAxis(p.vMax, p.acc, p.pullIn);
        float v = p.v;
        for (int i = 0; i < 10; i++)
        {
            a.rotate(v);
            long n = (long)(2.0 * p.v * p.v / p.acc) + 1'000; // enough steps to brake and to reach the speed again
            Run r = run(a, n);
            CHECK(a.speed() == approx(v).epsilon(0.0001));
            CHECK(r.reversals() == (i == 0 ? 0 : 1));
            checkKinematics(r, {p.vMax, p.acc, p.acc, p.pullIn});
            v = -v;
        }
    }
}

TEST_CASE("pattern 15 (#47): reversal at a low speed")
{
    Axis a = makeAxis(1'000, 2'500, 100);
    a.rotate(124);
    run(a, 200);
    a.stop();
    run(a);
    int32_t xStop = a.position();
    CHECK(xStop < 210);
    a.rotate(-124);
    run(a, 200);
    CHECK(a.speed() == approx(-124).epsilon(0.0001));
    CHECK(a.position() == xStop - 200);
}

TEST_CASE("pattern 16 (#49, #55, #92): the speed reads 0 after a stop and after an emergency stop")
{
    Axis a = makeAxis(5'000, 50'000);
    a.rotate(5'000);
    run(a, 1'000);
    a.emergencyStop();
    CHECK(a.speed() == 0.0f);

    a.moveAbs(a.position() + 2'000);
    run(a);
    CHECK(a.speed() == 0.0f);
}

TEST_CASE("pattern 17 (#139): chained moves with decreasing speed keep stepping")
{
    Axis a = makeAxis(20'000, 100'000);
    int32_t x = 0;
    for (float v : {20'000.0f, 10'000.0f, 5'000.0f, 1'000.0f, 100.0f})
    {
        x += 5'000;
        a.setMaxSpeed(v);
        a.moveAbs(x, v);
        Run r = run(a, 20'000'000, Until::arrived);
        CHECK(a.position() == x);
        CHECK(a.speed() == approx(v).epsilon(0.0001));
        CHECK(r.steps() == 5'000);
    }
}

TEST_CASE("pattern 19 (forum): a stop during the deceleration does not accelerate again")
{
    Axis a = makeAxis(10'000, 20'000);
    a.moveAbs(30'000);
    run(a, 28'500); // decelerating
    float v = a.speed();
    a.stop();
    Run r = run(a);
    for (auto& e : r.s) REQUIRE(e.v <= v);
    CHECK(a.position() <= 30'000);
}
