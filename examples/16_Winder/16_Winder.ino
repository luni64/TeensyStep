/*==========================================================================
 * 16 Winder: two motors with a ratio that can be adjusted while running
 *
 * A coil winder: the spindle turns the core, the feeder moves the wire along
 * it. The motors run independently, so that the pitch can be trimmed.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

// mechanics
constexpr float spindleStpPerRev = 200 * 16;       // full steps * microstepping
constexpr float feederStpPerMM   = 200 * 8 * 5.0f; // full steps * microstepping * turns of the lead screw per mm
constexpr float spindleAcc       = 30'000;         // steps/s^2
constexpr float feederTrimAcc    = 100'000;        // steps/s^2, used for pitch changes only

Stepper spindle(0, 1);
Stepper feeder(2, 3);

float spindleSpeed = 0; // steps/s
float ratio        = 0; // feeder steps per spindle step

float toRatio(float pitch_mm) { return pitch_mm * feederStpPerMM / spindleStpPerRev; }

// new spindle speed, same pitch: both motors ramp together
void setSpindleSpeed(float rpm)
{
    spindleSpeed = rpm / 60 * spindleStpPerRev;

    feeder.setAcceleration(spindleAcc * ratio); // both ramps take the same time
    spindle.rotateAsync(spindleSpeed);
    feeder.rotateAsync(spindleSpeed * ratio); // speed 0 stops the motor
}

// new pitch, same spindle speed: only the feeder changes
void setPitch(float pitch_mm)
{
    ratio = toRatio(pitch_mm);

    feeder.setAcceleration(feederTrimAcc);
    feeder.rotateAsync(spindleSpeed * ratio);
}

void setup()
{
    spindle.setAcceleration(spindleAcc);

    setPitch(0.10f);       // mm per turn
    setSpindleSpeed(1125); // rpm: start winding

    delay(3000);
    setPitch(0.12f); // trim the pitch while winding
    delay(500);
    setPitch(0.08f);
    delay(500);
    setPitch(0.15f);

    delay(3000);
    setSpindleSpeed(300); // slow down, pitch stays at 0.15 mm

    delay(2500);
    setSpindleSpeed(0); // stop
}

void loop()
{
    if (spindle.isMoving()) // print time, both speeds and the actual pitch, e.g. for a diagram
    {
        float pitch = feeder.getSpeed() / spindle.getSpeed() * spindleStpPerRev / feederStpPerMM;
        Serial.printf("%u\t%.0f\t%.0f\t%.3f\n", millis(), spindle.getSpeed(), feeder.getSpeed(), pitch);
    }
    delay(25);
}
