/*==========================================================================
 * 14 Follower: a motor follows a calculated path
 *
 * A spindle rotates at constant speed. A slide moves in and out depending
 * on the angle of the spindle:  r = amplitude * cos(k * angle).
 * A pen on the slide then draws a rose curve on the rotating disk.
 *
 * Every 25 ms a timer calculates where the slide has to be 25 ms later and
 * how fast it has to move there, and gives both to the slide:
 *
 *     slide.moveAbsAsync(position, speed);
 *
 * Because the target includes the speed, the slide does not stop at each
 * point but runs through the points smoothly.
 *
 * The commands of the library may be called from a timer interrupt, as it
 * is done here. Give such a timer a low priority, so that it does not
 * delay the step pulses.
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
float slideSpeed(float phi) { return -amplitude * k * omega * sinf(k * phi); }

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
