/*==========================================================================
 * GroupWaypoints: a group passes several points without stopping
 *
 * advanced::GroupPath (TeensyStepAdvanced.h) takes moves ahead: one runs and
 * one waits. Each move says how long it is in mm; its speeds are in mm/s.
 *==========================================================================*/

#include "TeensyStepAdvanced.h"
using namespace TeensyStep;

Stepper x(2, 3);
Stepper y(4, 5);
StepperGroup xy{x, y};
advanced::GroupPath path(xy);

const float stepsPerMm = 100;

struct Waypoint
{
    float x, y;           // mm
    float speed;          // mm/s on the way to this point
    float speedAtThePoint; // mm/s; 0: stop there
};

const Waypoint waypoints[] = {
    {100, 0, 80, 5},
    {104, 4, 80, 5}, // corner, cut by a short diagonal move
    {104, 80, 80, 5},
    {0, 0, 80, 0}, // back to the start and stop
};
const unsigned nrOfWaypoints = sizeof(waypoints) / sizeof(waypoints[0]);

unsigned next = 0;
float fromX = 0, fromY = 0; // where the move that is given next starts

void giveMoves() // gives the library as many moves as it takes
{
    while (next < nrOfWaypoints && path.room() > 0)
    {
        const Waypoint& w = waypoints[next];

        advanced::PathMove move;
        move.length        = hypotf(w.x - fromX, w.y - fromY); // in mm, so the speeds are in mm/s
        move.speed         = w.speed;
        move.speedAtTarget = w.speedAtThePoint;
        path.add({(int32_t)lroundf(w.x * stepsPerMm), (int32_t)lroundf(w.y * stepsPerMm)}, move);

        fromX = w.x;
        fromY = w.y;
        next  = next + 1;
    }
}

void setup()
{
    x.setMaxSpeed(15'000).setAcceleration(30'000); // what the motors can do
    y.setMaxSpeed(15'000).setAcceleration(30'000);
}

void loop()
{
    giveMoves(); // the next move has to be there before the one that runs arrives
}
