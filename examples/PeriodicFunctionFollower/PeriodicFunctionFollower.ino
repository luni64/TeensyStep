#include "TeensyStep.h"
#include "PeriodicFunctions.h"

PathStepper spindle(0, 1);  // define spindle and slide steppers
PathStepper slide(2, 3);
PathControl controller(50); 

RoseFunction roseFunction(spindle,slide);  // RoseFunction object

void setup()
{  
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(4, OUTPUT);

  roseFunction
      .setParameters(7, 6)     // n=7 d=6 (see https://en.wikipedia.org/wiki/Rose_(mathematics))       
      .setRPM(5);              // spindle speed

  controller.followPathAsync(spindle,slide);
}

void loop()
{
  Serial.printf("%ld\t%ld\n", spindle.getPosition(), slide.getPosition()); // print current motor positions
  digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
  delay(20);
}
