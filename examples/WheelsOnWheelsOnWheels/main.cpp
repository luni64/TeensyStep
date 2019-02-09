#include "Arduino.h"
#include <cmath>
#include "TeensyStep.h"

PathStepper slide(0, 1), spindle(2, 3);

PathControl pathControl(20);
StepControl stepControl;

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
  pinMode(4, OUTPUT);

  printf("start\n");

  slide
      .setMaxSpeed(15'000)
      .setAcceleration(10'000);      

  spindle
      .setMaxSpeed(20'000)
      .setAcceleration(10'000);

  slide.attachFunction(slideFunc);
  slide.setScale(1500);
  spindle.attachFunction(spindleFunc);
  spindle.setScale(150);

  spindle.setPosition(1000*spindleFunc(0));
  slide.setPosition(1000*slideFunc(0));

  pathControl.followPathAsync(spindle, slide);

  // delay(5000);
  // flexControl.stop();

  // slide.setTargetAbs(0);
  // stepControl.move(slide);
}

void loop()
{
  //printf("{XYPLOT|DATA|rose|%ld|%ld}\n", spindle.getPosition(), slide.getPosition());

  printf("%ld\t%ld\n", spindle.getPosition(), slide.getPosition());
  digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
  delay(20);
}

extern "C"
{
  int _write(int handle, char *buf, int count)
  {
    return Serial.write(buf, count);
  }
}
