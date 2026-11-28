/*==========================================================================
 * 06 TargetWithSpeed: pass a position at a given speed, and chain moves
 *
 * The motor passes 10000 at 2000 steps/s and 40000 at 12000 steps/s, and
 * stops at 50000.
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

void nextWaypoint() // called at every arrival, from an interrupt: keep it short
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
