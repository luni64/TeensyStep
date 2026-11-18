/*==========================================================================
 * 01 HelloStepper: the smallest possible sketch
 *
 * STEP on pin 2, DIR on pin 3 (any pins work).
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3); // STEP pin 2, DIR pin 3

void setup()
{
}

void loop()
{
    motor.moveRel(1000); // 1000 steps forward
    delay(500);

    motor.moveRel(-1000); // and back
    delay(500);
}
