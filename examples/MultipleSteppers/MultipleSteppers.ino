/*==========================================================================
 * This sketch shows how to move more than one motor in sync. 
 * 
 *  
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

Stepper motor_1(2, 3);
Stepper motor_2(9,10);
Stepper motor_3(14,15);

StepControl<> controller;

void setup()
{
  // setup the used motors 

   motor_1
    .setMaxSpeed(50000)       // steps/s
    .setAcceleration(200000); // steps/s^2 
  
  motor_2
    .setMaxSpeed(50000)       // steps/s
    .setAcceleration(200000); // steps/s^2 

  motor_3
    .setPullInSpeed(300)      // steps/s   
    .setMaxSpeed(10000)       // steps/s
    .setAcceleration(50000)   // steps/s^2     
	.setStepPinPolarity(LOW); // driver expects active low pulses
}

void loop() 
{  
  constexpr int spr = 3200;  // 16*200=3200 steps per revolution
  
  // lets shake    
  for(int i = 0; i < 5; i++)
  {
    motor_1.setTargetRel(spr/4); // 1/4 revolution
    controller.move(motor_1);  

    motor_1.setTargetRel(-spr/4);
    controller.move(motor_1);  
  }
  delay(500);
  
  // move motor_1 to absolute position (10 revolutions from zero)
  // move motor_2 half a revolution forward  
  // both motors will arrive their target positions at the same time
  motor_1.setTargetAbs(10*spr);
  motor_2.setTargetRel(spr/2);
  controller.move(motor_1, motor_2);

  // now move motor_2 together with motor_3
  motor_2.setTargetRel(300);
  motor_3.setTargetRel(-800);
  controller.move(motor_2, motor_3);

  // move motor_1 and motor_3 back to their start positions
  motor_1.setTargetAbs(0);
  motor_3.setTargetAbs(0);
  controller.move(motor_1, motor_3);
 
  delay(1000);
}
