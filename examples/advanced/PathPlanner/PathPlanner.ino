/*==========================================================================
 * PathPlanner: an x-y table runs a path without stopping at its points
 *
 * An example of the advanced functions (TeensyStepAdvanced.h). The path is
 * a list of points in mm: a rectangle and a circle of 72 short segments.
 * The planner in Planner.h, a part of this example, decides how fast each
 * point can be passed and gives the library one move for each segment.
 *
 * With the values below, the corners of the rectangle are passed at
 * 5 mm/s and the circle is run at 55 mm/s and more, of a feed rate of
 * 80 mm/s.
 * Both follow from `maxJump`, the change of speed a motor takes without a
 * ramp, and from the angle between two segments: a higher `maxJump`, or a
 * circle of more segments, is run faster.
 *
 * How the planner works: "Writing a motion planner" in the manual.
 *==========================================================================*/

#include "Planner.h" // includes TeensyStepAdvanced.h
using namespace TeensyStep;

Stepper x(2, 3);
Stepper y(4, 5);
StepperGroup xy{x, y};

// What the motors can do: maximum speed (steps/s), acceleration (steps/s^2), and the largest
// change of speed without a ramp (steps/s)
const planner::MotorLimits limits[2] = {
    {20'000, 50'000, 500}, // x
    {20'000, 50'000, 500}, // y
};

// One motor for each axis, 100 steps per mm
const planner::Kinematics table = planner::Kinematics::plain({100, 100});

planner::Segment segments[100]; // room for the path
planner::Path path(segments, 100, table, limits);
planner::PathRunner runner(xy);

void makePath()
{
    const float feed = 80; // mm/s

    path.start({0, 0}); // where the motors stand
    path.add({60, 0}, feed); // a rectangle
    path.add({60, 40}, feed);
    path.add({0, 40}, feed);
    path.add({0, 0}, feed);

    path.add({45, 20}, feed); // to a circle of 15 mm around (30, 20)
    for (int i = 1; i <= 72; i++)
    {
        float angle = i * 2 * PI / 72;
        path.add({30 + 15 * cosf(angle), 20 + 15 * sinf(angle)}, feed);
    }
    path.add({0, 0}, feed); // and back

    path.plan(); // calculates the speeds
}

void setup()
{
    x.setMaxSpeed(limits[0].maxSpeed).setAcceleration(limits[0].acceleration); // the same limits as the planner has
    y.setMaxSpeed(limits[1].maxSpeed).setAcceleration(limits[1].acceleration);

    makePath();
    runner.start(path);
}

void loop()
{
    runner.giveMoves(); // the next move has to be there before the one that runs arrives

    if (!runner.isRunning()) // the path is done: again after a second
    {
        delay(1000);
        runner.start(path);
    }
}
