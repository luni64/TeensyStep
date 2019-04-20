#include "Encoder.h"
#include "TeensyStep.h"
#include "Winder.h"

constexpr unsigned feederStpPerMM = 200 * 8 * 5.0f * 0.8f; // e.g. fullstep/rev * microstepping * leadscrew pitch * gearRatio
constexpr unsigned spindleStpPerRev = 200 * 16;            // e.g. fullstep/rev * microstepping
constexpr unsigned spindleAcceleration = 15000;

Stepper spindle(0, 1);
Stepper feeder(2, 3);
Winder winder(spindle, feeder); 

float pitch = 0.2; //mm

Encoder trimmer(5, 6);
int oldTrimmerVal = 0;

IntervalTimer printTimer;                                  // used to print out current speeds in the background

void setup()
{
  while (!Serial && millis() < 500);

  // setup background printing
  printTimer.begin(printCurrent, 25000);
  printTimer.priority(255); // lowest priority, we don't want to disturb stepping

  // setup the winder
  winder
      .setSpindleParams(spindleStpPerRev, spindleAcceleration)
      .setFeederParams(feederStpPerMM, 50000)
      .begin();

  // startup the winder
  winder.setSpindleSpeed(600); // spindle speed in rpm
  winder.setPitch(0.2f);       // pitch in mm
  winder.updateSpeeds();       // apply new settings
  delay(3000);
}

void loop()
{
  // read in the trimmer and adjust the pitch (+/- 0.01 per encoder detent)
  int trimmerVal = trimmer.read(); 
  if (trimmerVal != oldTrimmerVal)
  {
    oldTrimmerVal = trimmerVal;
    pitch = 0.2f + trimmerVal / 400.0f;  // e.g. 0.01 per 4 encoder steps
    winder.setPitch(pitch);
    winder.updateSpeeds();
  }

  if (millis() > 15000)                 // stop the spindle after 15s
  {
    winder.setSpindleSpeed(0);
    winder.updateSpeeds();
    while (1)
      yield();                          // stop sketch here
  }

  delay(50);                            // dont overrun the winder by sending new values to often. 
}

// helpers ----------------------------------------------

void printCurrent()
{
  unsigned t = millis();
  unsigned feederSpeed = winder.getCurFeederSpeed();
  unsigned spindleSpeed = winder.getCurSpindleSpeed();
  float curPitch = winder.getCurPitch();

  if (spindleSpeed != 0 || feederSpeed != 0)
  {
      Serial.printf("%d\t%i\t%i\t%.3f\t%.3f\n", t, spindleSpeed, feederSpeed, curPitch, pitch);     
  }
}