/*==========================================================================
 * 07 Groups: move several motors synchronized
 *
 * The motors of a StepperGroup start together, arrive together and stay on
 * a straight line between start and target at all times (also while
 * accelerating and when the group is stopped early). The group runs as
 * fast as possible without any motor exceeding its own maximum speed or
 * acceleration.
 *
 * There are two ways to give the targets:
 *   a) all in one call, in the order of the motors in the group
 *   b) on each motor with setTargetAbs() / setTargetRel(), then move the group
 *
 * Groups are light objects. They can be defined once or created where they
 * are needed, and a motor can be a member of several groups (but only one
 * of them can move at a time).
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper x(2, 3);
Stepper y(4, 5);
Stepper z(6, 7);

StepperGroup xy{x, y};
StepperGroup xyz{x, y, z};

void setup()
{
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    z.setMaxSpeed(5'000).setAcceleration(10'000);

    // a) targets in one call
    xy.moveAbs({4'000, 1'000}); // x -> 4000, y -> 1000, on a straight line

    // b) targets on the motors
    x.setTargetAbs(0);
    y.setTargetRel(-500);
    z.setTargetAbs(2'000);
    xyz.move(); // blocks until all three have arrived

    // a group created on the fly
    StepperGroup{y, z}.moveAbs({0, 0});

    // non-blocking, with an early stop: the motors decelerate together and stay on the line
    xyz.moveAbsAsync({100'000, 50'000, 10'000});
    delay(1000);
    xyz.stop();

    // a motor of a group can still be moved on its own
    z.moveAbs(0);
}

void loop()
{
}
