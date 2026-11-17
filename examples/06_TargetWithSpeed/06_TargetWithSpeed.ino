/*==========================================================================
 * 06 TargetWithSpeed: pass a position at a given speed, and chain moves
 *
 * moveAbsAsync(p, v) has an optional second parameter: the speed the motor
 * shall have when it reaches p.
 *   moveAbsAsync(p)       stop at p               (same as v = 0)
 *   moveAbsAsync(p, v)    pass p at v steps/s and keep running at v
 *
 * The motor always reaches p exactly at speed v. If p is too close for
 * that, it takes a run-up (it may reverse first).
 *
 * onArrival() registers a function that is called when the target is
 * reached. It can give the next command, which then follows without a
 * stop. Here the motor runs a speed profile along a track:
 *
 *   position      0 ... 10000 ... 40000 ... 50000
 *   speed         0     2000      12000     0
 *
 * NOTE: the function is called from an interrupt. Keep it short and do not
 * use Serial or delay() in it.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

struct Waypoint
{
    int32_t position; // steps
    float speed;      // steps/s at this position
};

const Waypoint track[] = {
    {10'000, 2'000},  // slow section: pass 10000 at 2000 steps/s
    {40'000, 12'000}, // fast section: pass 40000 at 12000 steps/s
    {50'000, 0},      // stop at the end
};
const unsigned nrOfWaypoints = sizeof(track) / sizeof(track[0]);

volatile unsigned next = 0;

void nextWaypoint() // called at every arrival
{
    if (next < nrOfWaypoints)
    {
        motor.moveAbsAsync(track[next].position, track[next].speed);
        next = next + 1;
    }
}

void setup()
{
    motor.setMaxSpeed(15'000).setAcceleration(20'000);
    motor.onArrival(nextWaypoint);

    nextWaypoint(); // start with the first waypoint
}

void loop()
{
}
