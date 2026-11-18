/*==========================================================================
 * 12 SyncRotation: rotate several motors with a fixed speed ratio
 *
 * The second motor always makes 3 steps while the first makes 1.
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
