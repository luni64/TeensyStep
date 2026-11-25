/*==========================================================================
 * 17 LegacyV2: sketches written for TeensyStep 2 still compile
 *
 * StepControl and RotateControl of version 2 are kept as thin wrappers
 * around the new functions. They are deprecated: the compiler shows a
 * hint, and new sketches should use the commands on the motor directly.
 *
 * A version 2 sketch needs no "using namespace TeensyStep;": the header
 * provides the old global names Stepper, StepControl and RotateControl.
 * (Define TEENSYSTEP_NO_GLOBAL_NAMES before the include to switch them
 * off, e.g. if another library also has a class named Stepper.)
 *
 *   version 2                                new
 *   -------------------------------------    ---------------------------------
 *   StepControl c;                           (not needed)
 *   m.setTargetAbs(1000); c.move(m);         m.moveAbs(1000);
 *   c.moveAsync(m1, m2);                     StepperGroup{m1, m2}.moveAsync();
 *   RotateControl r; r.rotateAsync(m);       m.rotateAsync(speed);
 *   r.overrideSpeed(0.5);                    m.rotateAsync(newSpeed);
 *   c.isRunning()                            m.isMoving()
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
