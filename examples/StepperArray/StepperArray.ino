#include <TeensyStep.h>

Stepper M1(0, 1), M2(2, 3), M3(4, 5), M4(6, 7), M5(8, 9), M6(10, 11); // create 6 motors
StepControl controller;

void setup() 
{
  pinMode(LED_BUILTIN,OUTPUT);
  delay(100);
  
  Stepper* motorSet_A[] = {&M1, &M2, &M5, &M6  };  // define an array of pointers to motors 1,2,5 and 6
  M1.setTargetAbs(200);                            // set targets for the motors
  M2.setTargetAbs(-500);   
  M5.setTargetAbs(-100);
  M6.setTargetAbs(30);  
  controller.move(motorSet_A);                     // move all motors in the array to their targets

  delay(500);                                      // just to generate a nice output on the logic analyzer
 
  Stepper* motorSet_B[] = {&M1, &M3, &M4};         // another set of motors;
  M1.setTargetAbs(-110);
  M3.setTargetAbs(50);   
  M4.setTargetAbs(230);
  controller.move(motorSet_B);                     // move set B to target

  delay(500);                                      // just to generate a nice output on the logic analyzer
 
  for (int i = 0; i < 4; i++) {                    // loop through all motors in motorSet_A...
    motorSet_A[i]->setTargetAbs(0);                // ... set targets to 0...
  }
  controller.move(motorSet_A);                     // ... and move home
}

void loop(){
  digitalWriteFast(LED_BUILTIN,!digitalReadFast(LED_BUILTIN));
  delay(100);
}