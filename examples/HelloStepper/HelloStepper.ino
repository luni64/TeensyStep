/*==========================================================================
 * This is a minimal sketch showing the usage of TeensyStep
 *  
 * STEP Pulses on Pin 2    (can be any pin)
 * DIR  Signall on Pin 3   (can be any pin)
 * 
 * The target position is set to 1000 steps relative to the
 * current position. The move command of the controller 
 * moves the motor to the target position.  
 *  
 * Default parameters are 
 * Speed:          800 steps/s
 * Acceleration:  2500 steps/s^2
 * 
 * (slow, but good to start with since they will work on any normal stepper)
 *
 ===========================================================================*/

#include "TeensyStep.h"

Stepper motor(2, 3);       // STEP pin: 2, DIR pin: 3
StepControl controller;    // Use default settings 

void setup()
{
}

void loop() 
{
  motor.setTargetRel(1000);  // Set target position to 1000 steps from current position
  controller.move(motor);    // Do the move
  delay(500);
}
