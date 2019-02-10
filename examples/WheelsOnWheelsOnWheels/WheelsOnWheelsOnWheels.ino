#include "Arduino.h"
#include <cmath>
#include "TeensyStep.h"

PathStepper slide(0, 1), spindle(2, 3);
PathControl pathControl(20);

constexpr float omega = 0.00000004f;

float spindleFunc(float t)
{
  float phi = fmodf(omega * t, 6.28);
  
  //return sinf(phi) + 0.5f*sinf(7.0f*phi) + 0.3333f*cosf(17.0f*phi) ;
  return sinf(-2.0f * phi) + 0.5f*sinf(5.0f*phi) + 0.25*sinf(19.0f*phi) ;
}

float slideFunc(float t)
{
  float phi = fmodf(omega * t, 6.28);
  //return cosf(phi) + 0.5f*cosf(7.0f*phi) + 0.33333f*sinf(17.0f*phi) ;
  return cosf(-2.0f*phi) + 0.5f*cosf(5.0f*phi) + 0.25f*cosf(19.0f*phi) ;    
}

void setup()
{
  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(4, OUTPUT); // test pin
  
  slide.attachFunction(slideFunc);
  slide.setScale(1500);

  spindle.attachFunction(spindleFunc);
  spindle.setScale(150);

  spindle.setPosition(1000*spindleFunc(0));  // hack
  slide.setPosition(1000*slideFunc(0));

  pathControl.followPathAsync(spindle, slide);
}

void loop()
{
  Serial.printf("%ld\t%ld\n", spindle.getPosition(), slide.getPosition());
  digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
  delay(20);
}

