// Sketches written for TeensyStep 2: the global names Stepper, StepControl and RotateControl, used as in the
// examples of version 2 (R2). The tests read the simulated pins.
#include "TeensyStep.h"
#include "approx.h"
#include "backend/host/HostBackend.h"
#include "doctest.h"

#include <cmath>

// the controllers are deprecated; this file uses them on purpose
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace host = TeensyStep::backend::host;
namespace backend = TeensyStep::backend;

namespace
{
    const backend::MotorPins pinsA{2, 3, true, true};
    const backend::MotorPins pinsB{9, 10, true, true};
    const backend::MotorPins pinsC{5, 6, true, true};

    void delay(unsigned ms) { host::run(ms * 0.001); } // the Arduino function, on the simulated clock

    struct Board
    {
        Board() { host::reset(); }
    };
}

// no "using namespace TeensyStep" from here on: version 2 sketches have none

TEST_CASE("version 2: HelloStepper")
{
    Board board;
    Stepper motor(2, 3);    // STEP pin: 2, DIR pin: 3
    StepControl controller; // Use default settings

    for (int i = 0; i < 2; i++) // loop()
    {
        motor.setTargetRel(1000); // Set target position to 1000 steps from current position
        controller.move(motor);   // Do the move
        delay(500);
    }
    CHECK(motor.getPosition() == 2000);
    host::Pulses p = host::pulses(pinsA);
    CHECK(p.net == 2000);
    CHECK(p.minPeriod == approx(1.0 / 800).epsilon(0.001));
    CHECK(host::openChannels() == 0);
}

TEST_CASE("version 2: synchronized moves, motors as a list and as an array, with a speed factor")
{
    Board board;
    Stepper motor_1(2, 3), motor_2(9, 10), motor_3(5, 6);
    StepControl controller;

    motor_1.setMaxSpeed(50000).setAcceleration(200000);
    motor_2.setMaxSpeed(50000).setAcceleration(200000);
    motor_3.setMaxSpeed(50000).setAcceleration(200000).setPullInSpeed(300).setStepPinPolarity(1).setInverseRotation(false);

    motor_1.setTargetRel(1000);
    motor_2.setTargetRel(-2000);
    controller.move(motor_1, motor_2); // synchronized
    CHECK(host::pulses(pinsA).net == 1000);
    CHECK(host::pulses(pinsB).net == -2000);
    CHECK(host::pulses(pinsA).last <= host::pulses(pinsB).last + 1e-12); // arrived together

    Stepper* motorSet[] = {&motor_1, &motor_2, &motor_3};
    motor_1.setTargetAbs(0);
    motor_2.setTargetAbs(0);
    motor_3.setTargetAbs(-40000);
    controller.move(motorSet);
    CHECK(motor_1.getPosition() == 0);
    CHECK(motor_2.getPosition() == 0);
    CHECK(motor_3.getPosition() == -40000);
    CHECK(host::pulses(pinsC).minPeriod == approx(1.0 / 50000).epsilon(0.001));

    motor_3.setTargetAbs(0);
    double start = host::now();
    controller.move(0.25f, motor_3); // a quarter of the maximum speed
    CHECK(host::pulses(pinsC, start).minPeriod == approx(1.0 / 12500).epsilon(0.001));
    CHECK(motor_3.getPosition() == 0);
}

namespace
{
    int done = 0;
    void onDone() { done++; }
}

TEST_CASE("version 2: non-blocking move, isRunning, stopAsync, emergencyStop, callback")
{
    Board board;
    Stepper motor(2, 3);
    StepControl controller(20, 1000); // pulse width 20 us; the update period is not used any more
    motor.setMaxSpeed(5000).setAcceleration(50000);
    done = 0;
    controller.setCallback(onDone);

    motor.setTargetAbs(20000);
    controller.moveAsync(motor);
    CHECK(controller.isRunning());
    delay(1000);
    CHECK(controller.getCurrentSpeed() == 5000);
    CHECK(host::pulses(pinsA).minWidth == approx(20e-6).epsilon(0.001));

    motor.setTargetAbs(0);
    controller.moveAsync(motor); // a running controller takes no new move
    delay(100);
    CHECK(motor.getPosition() > 5000);

    controller.stopAsync();
    while (controller.isRunning()) delay(10);
    CHECK(done == 0); // stopped before the target
    CHECK(motor.getPosition() == host::pulses(pinsA).net);
    CHECK(controller.getCurrentSpeed() == 0);

    motor.setTargetAbs(0);
    controller.move(motor);
    CHECK(done == 1);
    CHECK(motor.getPosition() == 0);

    motor.setTargetAbs(20000);
    controller.moveAsync(motor);
    delay(500);
    controller.emergencyStop();
    CHECK_FALSE(controller.isRunning());
    CHECK(motor.getPosition() == host::pulses(pinsA).net);
}

TEST_CASE("version 2: rotation, the sign of the maximum speed is the direction, speed factor also above 1")
{
    Board board;
    Stepper motor(2, 3);
    RotateControl controller;
    motor.setMaxSpeed(-1000).setAcceleration(20000);

    controller.rotateAsync(motor);
    delay(500);
    CHECK(controller.isRunning());
    CHECK(controller.getCurrentSpeed() == -1000);
    CHECK(host::pulses(pinsA).up == 0);

    controller.overrideSpeed(0.5f);
    delay(500);
    CHECK(controller.getCurrentSpeed() == -500);

    controller.overrideSpeed(3.0f); // above the "maximum" speed, as in version 2
    delay(500);
    CHECK(controller.getCurrentSpeed() == -3000);

    controller.overrideSpeed(-1.0f); // reverses
    delay(1000);
    CHECK(controller.getCurrentSpeed() == 1000);

    controller.overrideSpeed(0); // parks the motor
    delay(500);
    CHECK(motor.getPosition() == host::pulses(pinsA).net);
    long parked = host::pulses(pinsA).count;
    delay(500);
    CHECK(host::pulses(pinsA).count == parked);

    controller.overrideSpeed(1.0f); // and starts it again
    delay(500);
    CHECK(controller.getCurrentSpeed() == -1000);
    controller.stop();
    CHECK_FALSE(controller.isRunning());
    CHECK(motor.getPosition() == host::pulses(pinsA).net);
}

TEST_CASE("version 2: synchronized rotation of two motors, as in the documentation")
{
    Board board;
    Stepper motor_1(2, 3), motor_2(9, 10);
    RotateControl controller;
    motor_1.setAcceleration(10000);
    motor_1.setMaxSpeed(5000);
    motor_2.setAcceleration(10000);
    motor_2.setMaxSpeed(15000);

    controller.rotateAsync(motor_1, motor_2); // start synced rotation of both motors
    delay(5000);
    controller.stop();
    host::Pulses p1 = host::pulses(pinsA), p2 = host::pulses(pinsB);
    CHECK(p2.net > 50000);
    CHECK(std::abs(p2.net - 3 * p1.net) <= 2); // the ratio is kept, also while accelerating and stopping
    CHECK(p2.minPeriod == approx(1.0 / 15000).epsilon(0.001));
}

TEST_CASE("version 2: the winder example, speed 1 as base so that the factor is the speed")
{
    Board board;
    Stepper spindle(2, 3), feeder(9, 10);
    RotateControl feederCtrl(5, 1000), spindleCtrl(5, 1000);
    spindle.setMaxSpeed(1);
    spindle.setAcceleration(30000);
    feeder.setMaxSpeed(1);
    feeder.setAcceleration(30000);

    feederCtrl.rotateAsync(feeder);
    feederCtrl.overrideSpeed(0.0f);
    spindleCtrl.rotateAsync(spindle);
    spindleCtrl.overrideSpeed(0.0f);
    delay(100);

    feederCtrl.overrideAcceleration(0.2f); // both ramps take the same time
    feederCtrl.overrideSpeed(1200);
    spindleCtrl.overrideSpeed(6000);
    delay(100); // on the ramp: 6000 / 30000 = 0.2 s
    CHECK(spindleCtrl.getCurrentSpeed() == approx(3100).epsilon(0.02)); // pull-in speed 100 + 30000 * 0.1
    CHECK(feederCtrl.getCurrentSpeed() == approx(700).epsilon(0.02));
    delay(400);
    CHECK(spindleCtrl.getCurrentSpeed() == 6000);
    CHECK(feederCtrl.getCurrentSpeed() == 1200);

    spindleCtrl.overrideSpeed(0);
    feederCtrl.overrideSpeed(0);
    delay(500);
    CHECK(spindle.getPosition() == host::pulses(pinsA).net);
    CHECK(feeder.getPosition() == host::pulses(pinsB).net);
}
