/*==========================================================================
 * 13 Follower: a motor follows a calculated path
 *
 * The motor swings back and forth like a pendulum (a cosine). A timer
 * calculates every 10 ms where the motor has to be at the next call and
 * how fast it moves there.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);
IntervalTimer timer;

constexpr float dt        = 0.01f;         // s, period of the calculation
constexpr float amplitude = 5'000;         // steps
constexpr float omega     = TWO_PI * 0.5f; // rad/s: one swing forth and back in 2 s

void tick() // called every 10 ms
{
    static float t = 0;
    t += dt; // time of the next call

    float position = amplitude * cosf(omega * t);
    float speed    = -amplitude * omega * sinf(omega * t);
    motor.moveAbsAsync(position, speed); // be there, at this speed
}

void setup()
{
    motor.setMaxSpeed(20'000).setAcceleration(100'000);
    motor.moveAbs(amplitude); // to the start of the path

    timer.priority(255); // lowest priority: the calculation must not delay the step pulses
    timer.begin(tick, dt * 1'000'000);
}

void loop()
{
}
