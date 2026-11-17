/*==========================================================================
 * 08 GroupWaypoints: a group passes several points without stopping
 *
 * >>> OPEN DECISION: whether groups get a speed at the target and an
 * >>> arrival callback like single motors (example 06) is not decided yet.
 *
 * The second parameter of a group move is the speed at the target, given
 * for the motor with the longest way in this segment (the lead motor).
 * Within each segment the motors stay on a straight line. At a waypoint the
 * direction of the line changes, so the speeds of the individual motors
 * jump there. Choose the speed at the waypoints low enough for the motors
 * to follow; the library does not plan ahead across segments.
 *
 * The group cannot be given a new target in the middle of a segment; the
 * next target is set when it arrives (onArrival).
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper x(2, 3);
Stepper y(4, 5);
StepperGroup xy{x, y};

struct Waypoint
{
    int32_t x, y; // steps
    float speed;  // steps/s of the lead motor at this point
};

const Waypoint path[] = {
    {10'000, 0, 3'000},
    {12'000, 2'000, 3'000}, // corner, cut in two short segments
    {12'000, 10'000, 3'000},
    {0, 0, 0}, // back to the start and stop
};
const unsigned nrOfWaypoints = sizeof(path) / sizeof(path[0]);

volatile unsigned next = 0;

void nextWaypoint() // called from an interrupt at every arrival: keep it short
{
    if (next < nrOfWaypoints)
    {
        xy.moveAbsAsync({path[next].x, path[next].y}, path[next].speed);
        next = next + 1;
    }
}

void setup()
{
    x.setMaxSpeed(15'000).setAcceleration(30'000);
    y.setMaxSpeed(15'000).setAcceleration(30'000);

    xy.onArrival(nextWaypoint);
    nextWaypoint();
}

void loop()
{
}
