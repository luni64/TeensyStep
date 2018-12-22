/*==========================================================================
 * The sketch shows how to move more than one motor. 
 * 
 * If more than one motor is moved by one controller all motors will arrive at 
 * their targets at the same time. E.g., if the motors are part of a 
 * x/y transport system, the transport move on a straight diagonal line to the
 * target coordinates.
 * 
 * The sketch also shows examples how the motor properties are set up
 *
 * A 1/16 microstep driver is assumed. You probably want to adjust speed, 
 * acceleration and distances if you are using a driver with another microstep 
 * resolution.
 ===========================================================================*/

#include "SCurveControl.h"

Stepper motor_1(11, 10);   //STEP pin =  2, DIR pin = 3
Stepper motor_2(9, 8);   //STEP pin =  9, DIR pin = 10
Stepper motor_3(7, 6);  //STEP pin = 14, DIR pin = 15


SCurveControl controller;

void setup()
{
  // setup the motors 
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  pinMode(22, OUTPUT);
  digitalWrite(22, HIGH);
  
   motor_1
    .setPullInSpeed(20000)      // steps/s 
    .setMaxSpeed(40000)       // steps/s
    .setAcceleration(20000); // steps/s^2 
  
  motor_2
    .setPullInSpeed(20000)
    .setMaxSpeed(40000)       // steps/s
    .setAcceleration(20000); // steps/s^2 
  motor_3
    .setPullInSpeed(300)      // steps/s   
    .setMaxSpeed(10000)       // steps/s
    .setAcceleration(50000)   // steps/s^2     
    .setStepPinPolarity(LOW); // driver expects active low pulses
}

void loop() 
{  
  constexpr int spr = 16*200;  // 3200 steps per revolution
  
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
  motor_2.setTargetRel(3000);
  motor_3.setTargetRel(-8000);
  controller.move(motor_2, motor_3);

  // move all motors back to their start positions
  motor_1.setTargetAbs(0);
  motor_2.setTargetAbs(0);
  motor_3.setTargetAbs(0);
  controller.move(motor_1, motor_2, motor_3);
 
  delay(1000);
}

