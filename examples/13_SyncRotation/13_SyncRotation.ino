/*==========================================================================
 * 13 SyncRotation: rotate several motors with a fixed speed ratio
 *
 * rotateAsync() of a group runs its motors at the given speeds (steps/s,
 * the sign is the direction). The ratio of the speeds is kept exactly at
 * all times, also while the motors accelerate, decelerate or reverse:
 * here the second motor always makes 3 steps while the first makes 1.
 *
 * overrideSpeed(f) changes the speed of the whole group to f times the
 * speeds given in rotateAsync(), keeping the ratio. Negative factors
 * reverse all motors.
 *
 * The ratio itself cannot be changed while the group rotates. For an
 * adjustable ratio see example 15 (Winder).
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper m1(2, 3);
Stepper m2(4, 5);
StepperGroup group{m1, m2};

void setup()
{
    m1.setAcceleration(20'000);
    m2.setAcceleration(60'000);

    group.rotateAsync({5'000, 15'000}); // m1: 5000 steps/s, m2: 15000 steps/s
    delay(3000);

    group.overrideSpeed(0.5f); // 2500 and 7500 steps/s
    delay(3000);

    group.overrideSpeed(-1.0f); // reverse: -5000 and -15000 steps/s
    delay(3000);

    group.stop(); // both decelerate together and stop at the same time
}

void loop()
{
}
