/*==========================================================================
 * 15 RoseFunction: a slide follows the angle of a spindle and draws a rose curve
 *
 * The spindle rotates at constant speed. Every 25 ms a timer gives the slide
 * its next position and the speed there:  r = amplitude * cos(k * angle).
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

// spindle
constexpr float spindleStpPerRev = 6400;
constexpr float spindleRPM       = 15;
constexpr float spindleSpeed     = spindleRPM / 60 * spindleStpPerRev; // steps/s
constexpr float omega            = spindleRPM / 60 * TWO_PI;           // rad/s

// slide
constexpr float amplitude = 10'000; // steps

// rose function r(phi) = amplitude * cos(k * phi) and its change per second
constexpr float k = 5.0f / 8.0f;
float slidePosition(float phi) { return amplitude * cosf(k * phi); }
float slideSpeed(float phi) { return fabsf(amplitude * k * omega * sinf(k * phi)); } // without a sign, see below

Stepper spindle(1, 2);
Stepper slide(3, 4);

IntervalTimer tickTimer;
constexpr float dt = 0.025f; // s, period of the calculation

void tick() // called every 25 ms
{
    float phiNow  = spindle.getPosition() * (TWO_PI / spindleStpPerRev); // angle of the spindle
    float phiNext = phiNow + omega * dt;                                 // angle at the next call

    slide.moveAbsAsync(slidePosition(phiNext), slideSpeed(phiNext));
}

void setup()
{
    spindle.setAcceleration(50'000);
    slide.setMaxSpeed(35'000).setAcceleration(150'000);

    slide.moveAbs(slidePosition(0));  // move the slide to its start position
    spindle.rotateAsync(spindleSpeed); // let the spindle run

    tickTimer.priority(255); // lowest priority: the calculation must not delay the step pulses
    tickTimer.begin(tick, dt * 1'000'000);
}

void loop()
{
    // print the angle [rad] and the slide position [steps], e.g. for a plot
    Serial.printf("%f\t%d\n", spindle.getPosition() * (TWO_PI / spindleStpPerRev), slide.getPosition());
    delay(20);
}
