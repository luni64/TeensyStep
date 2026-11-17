/*==========================================================================
 * 01 HelloStepper: the smallest possible sketch
 *
 * STEP pulses on pin 2, DIR signal on pin 3 (any pins work).
 *
 * The motor moves 1000 steps forward, waits, and moves back, forever.
 * The commands are given directly to the motor; no controller object is needed.
 *
 * Default parameters (slow, but they work with any normal stepper):
 *   max speed:      800 steps/s
 *   acceleration:  2500 steps/s^2
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3); // STEP pin 2, DIR pin 3

void setup()
{
}

void loop()
{
    motor.moveRel(1000); // move 1000 steps from the current position; returns when the motor has stopped
    delay(500);

    motor.moveRel(-1000);
    delay(500);
}
