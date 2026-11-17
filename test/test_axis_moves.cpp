// core::Axis: moves from standstill to a target, rotation, stop. Requirements R7, R12, R17, R23, R25.
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
}

TEST_CASE("axis: a new axis stands still at position 0")
{
    Axis a;
    StepEvent e;
    CHECK(a.position() == 0);
    CHECK(a.speed() == 0.0f);
    CHECK_FALSE(a.isMoving());
    CHECK_FALSE(a.next(e));
}

TEST_CASE("axis: a move ends exactly at the target and at standstill")
{
    for (int32_t target : {1, 2, 3, 10, 1000, 50'000, -1, -7, -50'000})
    {
        CAPTURE(target);
        Axis a = makeAxis(10'000, 20'000);
        CHECK(a.moveAbs(target) == errorCode::OK);
        CHECK(a.isMoving());
        Run r = run(a);
        CHECK(r.finished);
        CHECK(a.position() == target);
        CHECK(a.speed() == 0.0f);
        CHECK_FALSE(a.isMoving());
        CHECK(r.steps() == std::abs(target));
        CHECK(r.reversals() == 0);
        CHECK(r.arrival() == r.steps() - 1); // the last step carries the arrival flag
        checkKinematics(r, {10'000, 20'000, 20'000, 0});
    }
}

TEST_CASE("axis: a long move reaches the maximum speed and takes the time of a trapezoid profile")
{
    Axis a = makeAxis(10'000, 20'000);
    a.moveAbs(50'000);
    Run r = run(a);
    CHECK(r.peakSpeed() == approx(10'000).epsilon(0.0001));
    CHECK(r.duration() == approx(idealDuration(50'000, 10'000, 20'000)).epsilon(0.002));
}

TEST_CASE("axis: a short move takes the time of a triangle profile and stays below the maximum speed")
{
    Axis a = makeAxis(10'000, 20'000);
    a.moveAbs(2'000); // reaching 10000 steps/s would need 2500 steps for the ramp up alone
    Run r = run(a);
    CHECK(r.peakSpeed() < 6'400);
    CHECK(r.peakSpeed() > 6'200); // sqrt(a * dist) = 6325
    CHECK(r.duration() == approx(idealDuration(2'000, 10'000, 20'000)).epsilon(0.005));
}

TEST_CASE("axis: the first step from standstill comes after sqrt(2/a)")
{
    Axis a = makeAxis(10'000, 20'000);
    a.moveAbs(1000);
    Run r = run(a, 1);
    CHECK(r.s[0].dir == 1);
    CHECK(r.s[0].dt == approx(std::sqrt(2.0 / 20'000)).epsilon(0.001));
}

TEST_CASE("axis: moveRel is relative to the current position")
{
    Axis a = makeAxis(5'000, 50'000);
    a.setPosition(300);
    a.moveRel(-500);
    run(a);
    CHECK(a.position() == -200);
}

TEST_CASE("axis: a move to the current position makes no step")
{
    Axis a = makeAxis(5'000, 50'000);
    a.setPosition(42);
    CHECK(a.moveAbs(42) == errorCode::OK);
    Run r = run(a);
    CHECK(r.finished);
    CHECK(r.steps() == 0);
    CHECK_FALSE(a.isMoving());
}

TEST_CASE("axis: with a pull-in speed the motor starts and stops at that speed")
{
    Axis a = makeAxis(10'000, 20'000, 500);
    a.moveAbs(20'000);
    Run r = run(a);
    CHECK(a.position() == 20'000);
    // the motor starts at 500 steps/s and accelerates from there: the first step comes after about one
    // period at the pull-in speed, not after the long first period of a ramp from 0
    double v1 = std::sqrt(500.0 * 500.0 + 2 * 20'000.0);
    CHECK(std::fabs(r.s.front().v) == approx(v1).epsilon(0.001));
    CHECK(r.s.front().dt == approx(2.0 / (500.0 + v1)).epsilon(0.001));
    CHECK(std::fabs(r.s[r.s.size() - 2].v) < 550); // the last moving speed is near the pull-in speed
    CHECK(r.duration() < idealDuration(20'000, 10'000, 20'000));
    checkKinematics(r, {10'000, 20'000, 20'000, 500});
}

TEST_CASE("axis: a maximum speed below the pull-in speed is used without a ramp")
{
    Axis a = makeAxis(200, 20'000, 500);
    a.moveAbs(100);
    Run r = run(a);
    CHECK(a.position() == 100);
    for (long i = 0; i < r.steps() - 1; i++) CHECK(r.s[i].dt == approx(1.0 / 200).epsilon(0.001)); // also the first step
}

TEST_CASE("axis: separate deceleration")
{
    Axis a = makeAxis(10'000, 40'000);
    a.setDeceleration(10'000);
    a.moveAbs(30'000);
    Run r = run(a);
    CHECK(a.position() == 30'000);
    checkKinematics(r, {10'000, 40'000, 10'000, 0});

    // ramp up: v²/2a = 1250 steps, ramp down: v²/2d = 5000 steps
    long up = 0, down = 0;
    for (long i = 1; i < r.steps(); i++)
    {
        if (r.s[i].v > r.s[i - 1].v) up++;
        if (r.s[i].v < r.s[i - 1].v) down++;
    }
    CHECK(up == approx(1250).epsilon(0.01));
    CHECK(down == approx(5000).epsilon(0.01));
}

TEST_CASE("axis: setAcceleration sets the deceleration too, until setDeceleration is used")
{
    Axis a = makeAxis(10'000, 40'000);
    a.moveAbs(30'000);
    Run r = run(a);
    checkKinematics(r, {10'000, 40'000, 40'000, 0});
    CHECK(r.duration() == approx(idealDuration(30'000, 10'000, 40'000)).epsilon(0.002));
}

TEST_CASE("axis: rotation accelerates to the given speed and keeps it")
{
    for (float speed : {8'000.0f, -8'000.0f})
    {
        CAPTURE(speed);
        Axis a = makeAxis(10'000, 20'000);
        CHECK(a.rotate(speed) == errorCode::OK);
        Run r = run(a, 10'000);
        CHECK_FALSE(r.finished);
        CHECK(a.isMoving());
        CHECK(a.speed() == approx(speed).epsilon(0.0001));
        CHECK(a.position() == (speed > 0 ? 10'000 : -10'000));
        CHECK(r.s.back().dt == approx(1.0 / 8'000).epsilon(0.0001));
        checkKinematics(r, {10'000, 20'000, 20'000, 0});
    }
}

TEST_CASE("axis: stop decelerates to standstill; rotate(0) does the same")
{
    for (int variant = 0; variant < 2; variant++)
    {
        Axis a = makeAxis(10'000, 20'000);
        a.rotate(10'000);
        run(a, 10'000);
        int32_t x0 = a.position();
        if (variant == 0) a.stop();
        else a.rotate(0);
        Run r = run(a);
        CHECK(r.finished);
        CHECK(a.speed() == 0.0f);
        CHECK_FALSE(a.isMoving());
        CHECK(a.position() - x0 == approx(2500).epsilon(0.01)); // v²/2d
        checkKinematics(r, {10'000, 20'000, 20'000, 0});
    }
}

TEST_CASE("axis: emergency stop ends the motion at once")
{
    Axis a = makeAxis(10'000, 20'000);
    a.rotate(10'000);
    run(a, 5'000);
    a.emergencyStop();
    StepEvent e;
    CHECK(a.speed() == 0.0f);
    CHECK_FALSE(a.isMoving());
    CHECK_FALSE(a.next(e));
    CHECK(a.position() == 5'000);

    a.moveAbs(0); // a new command works directly afterwards
    run(a);
    CHECK(a.position() == 0);
}

TEST_CASE("axis: fractional speeds")
{
    Axis a = makeAxis(0.5f, 10);
    a.moveAbs(5);
    Run r = run(a);
    CHECK(a.position() == 5);
    CHECK(r.s[2].dt == approx(2.0).epsilon(0.001)); // 0.5 steps/s
}

TEST_CASE("axis: values outside the supported range are limited and reported")
{
    Axis a;
    CHECK(a.setMaxSpeed(5'000'000) == errorCode::speedLimited);
    CHECK(a.setMaxSpeed(0) == errorCode::speedLimited);
    CHECK(a.setMaxSpeed(-100) == errorCode::speedLimited);
    CHECK(a.setAcceleration(1e12f) == errorCode::accelerationLimited);
    CHECK(a.setAcceleration(0) == errorCode::accelerationLimited);
    CHECK(a.setDeceleration(-5) == errorCode::accelerationLimited);
    CHECK(a.setPullInSpeed(-1) == errorCode::speedLimited);

    a.setMaxSpeed(1'000);
    a.setAcceleration(20'000);
    a.setDeceleration(20'000);
    CHECK(a.rotate(5'000) == errorCode::speedLimited); // above the maximum speed: runs at the maximum
    run(a, 2'000);
    CHECK(a.speed() == approx(1'000).epsilon(0.0001));
    CHECK(a.moveAbs(0, 5'000) == errorCode::speedLimited);
}

TEST_CASE("axis: the position wraps around at the ends of the 32-bit range")
{
    Axis a = makeAxis(10'000, 100'000);
    a.setPosition(INT32_MAX - 10);
    a.rotate(1'000);
    run(a, 21);
    CHECK(a.position() == INT32_MIN + 10);

    // a target across the wrap is reached the short way
    a.emergencyStop();
    a.moveAbs(INT32_MAX - 4);
    Run r = run(a);
    CHECK(a.position() == INT32_MAX - 4);
    CHECK(r.steps() == 15);
}
