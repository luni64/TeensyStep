/*==========================================================================
 * 13 Groups: move several motors synchronized
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
    xy.moveAbs({4'000, 1'000}); // x -> 4000, y -> 1000, synchronized
    xy.moveRel({-1'000, 500});  // x 1000 steps back, y 500 on: x -> 3000, y -> 1500

    // b) targets on the motors
    x.setTargetAbs(0);
    y.setTargetRel(-500);
    z.setTargetAbs(2'000);
    xyz.move(); // blocks until all three have arrived

    // a group created on the fly
    StepperGroup{y, z}.moveAbs({0, 0});

    // a group is as fast as its motors allow; a factor makes it slower
    xy.overrideSpeed(0.25);
    xy.moveAbs({4'000, 1'000}); // at a quarter of that speed: x runs at 5000 steps/s, y at 1250
    xy.overrideSpeed(1);

    // non-blocking, with an early stop: the motors decelerate together and stay synchronized
    xyz.moveAbsAsync({100'000, 50'000, 10'000});
    delay(1000);
    xyz.stop();

    // a motor of a group can still be moved on its own
    z.moveAbs(0);
}

void loop()
{
}
