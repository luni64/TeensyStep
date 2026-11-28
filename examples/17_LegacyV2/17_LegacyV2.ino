/*==========================================================================
 * 17 LegacyV2: sketches written for TeensyStep 2 still compile
 *
 * StepControl and RotateControl of version 2 are kept as wrappers on the new
 * commands. They are deprecated: the compiler shows a hint for them.
 *==========================================================================*/

#include "TeensyStep.h"

Stepper motor_1(2, 3); // unchanged version 2 code from here on
Stepper motor_2(9, 10);
StepControl controller;
RotateControl rotator;

void setup()
{
    motor_1.setMaxSpeed(50'000).setAcceleration(200'000);
    motor_2.setMaxSpeed(50'000).setAcceleration(200'000);
}

void loop()
{
    motor_1.setTargetRel(1000);
    motor_2.setTargetRel(-2000);
    controller.move(motor_1, motor_2); // synchronized move

    rotator.rotateAsync(motor_1);
    delay(1000);
    rotator.overrideSpeed(0.5f);
    delay(1000);
    rotator.stop();
}
