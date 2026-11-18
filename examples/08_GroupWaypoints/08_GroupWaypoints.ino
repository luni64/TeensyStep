/*==========================================================================
 * 08 GroupWaypoints: a group passes several points without stopping
 *
 * The second parameter of a group move is the speed at the targets. It is
 * the speed of the motor with the longest way in this move, like the
 * maximum speed of a group (setMaxSpeed). The group passes its targets at
 * this speed and runs on along the line until the next command.
 *
 * The next move is given from the arrival function. It starts where the
 * motors are, so the group goes through the point without a stop.
 *
 * Within each move the motors stay on a straight line. At a waypoint the
 * direction of the line changes, so the speeds of the single motors jump
 * there: below, y starts at 500 steps/s when the group passes the first
 * point. Choose the speed at the waypoints low enough for the motors to
 * follow. The library does not plan ahead across the moves, and it does
 * not round corners; cut a corner into short moves, as in this path.
 *
 * New targets are accepted when the group has passed its targets (or stands
 * still), not in the middle of a move.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper x(2, 3);
Stepper y(4, 5);
StepperGroup xy{x, y};

struct Waypoint
{
    int32_t x, y; // steps
    float speed;  // steps/s at this point; 0: stop there
};

const Waypoint path[] = {
    {10'000, 0, 500},
    {10'400, 400, 500}, // corner, cut by a short diagonal move
    {10'400, 8'000, 500},
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
    x.setMaxSpeed(15'000).setAcceleration(30'000); // what the motors can do
    y.setMaxSpeed(15'000).setAcceleration(30'000);

    xy.setMaxSpeed(8'000); // what the group shall do: the motor with the longest way runs at 8000 steps/s

    xy.onArrival(nextWaypoint);
    nextWaypoint();
}

void loop()
{
}
