#include <cmath>
#include "TeensyStep.h"

// define spindle and slide stepper --------------------
constexpr int spindleStp = 2;
constexpr int spindleDir = 3;
constexpr int slideStp   = 0; 
constexpr int slideDir   = 1;

PathStepper slide(slideStp, slideDir);  
PathStepper spindle(spindleStp, spindleDir);

// we use a PathControl controller to move the motors along a path given
// by two functions 

PathControl pathControl(20);  // Evaluate path functions every 20µs (50kHz)

// The following function is called every 20µs (see line 16) 
// t = time in µs. It needs to return a value proprtional to the spindle angle at time t
//
// Simple example: constantly rotating spindle with 1 rpm
// float spindleFunctd(float t)
// {    
//   return (TWO_PI / 60) * (t / 1E6) ;  // t = 0 -> returns 0; 30s: t = 30'000'000µs -> return pi, 60s: t= 60'000'000 -> return 2pi
// }

// This next example generates a spindle oscillation according to the Farris paper
float spindleFunc(float t)  
{
  float phi = (TWO_PI / 60) * (t/1E6);  // 1rpm  
  phi = fmodf(phi, TWO_PI);             // limit phi to 2pi to avoid sin(large argument) which will calculate very long
  
  return sinf(-2.0f * phi) + 0.5f*sinf(5.0f*phi) + 0.25*sinf(19.0f*phi) ;  
  //return sinf(phi) + 0.5f*sinf(7.0f*phi) + 0.3333f*cosf(17.0f*phi) ;
}

// The following function is called every 20µs (see line 16) 
// t = time in µs. It needs to return a value proportional to the slide position at time t
//
// Simple example: constantly moving slide with 14 mm/s
// float slideFunctd(float t)
// {    
//   return (t / 1E6) * 14.0f ;  // t = 0 -> returns 0; 0.5s: t = 500'000µs -> returns 7, 1s: t= 1'000'000 -> returns 14
// }

// This next example generates a slide oscillation according to the Farris paper
float slideFunc(float t)
{
  float phi = (TWO_PI / 60) * (t/1E6);  // 1rpm  
  return cosf(-2.0f*phi) + 0.5f*cosf(5.0f*phi) + 0.25f*cosf(19.0f*phi) ;    
  //return cosf(phi) + 0.5f*cosf(7.0f*phi) + 0.33333f*sinf(17.0f*phi) ; 
}

void setup()
{  
  pinMode(LED_BUILTIN, OUTPUT);

  slide.attachFunction(slideFunc);      //attach a path function to the stepper
  slide.setScale(1500);                 //scaling factor of the movement. The result of slideFunc is multiplied by this factor to translate to steps

  spindle.attachFunction(spindleFunc);  //attach a path function to the stepper
  spindle.setScale(150);                //scaling factor of the movement. The result of spindleFunc is multiplied by this factor to translate to steps

  spindle.setPosition(150*spindleFunc(0));  // set starting values 
  slide.setPosition(1500*slideFunc(0));

  pathControl.followPathAsync(spindle, slide);  // start movment
}

void loop()
{
  Serial.printf("%ld\t%ld\n", spindle.getPosition(), slide.getPosition());  // print current motor positions 
  digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
  delay(20);
}
