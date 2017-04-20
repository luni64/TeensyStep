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

#include <StepControl.h>

Stepper motor(2, 3);
StepControl<> controller;

void setup(){
}

void loop() 
{
  motor.SetTargetRel(1000);
  controller.move(motor);  
  delay(500);
}
