/*==========================================================================
 * CoreXY: a planned path on a CoreXY machine, with override and hold
 *
 * Motor A = x + y, motor B = x - y; the planner in Planner.h gets this as its
 * kinematics. Serial monitor:  - slower   + faster   h hold   g go on
 *==========================================================================*/

#include "Planner.h" // includes TeensyStepAdvanced.h
using namespace TeensyStep;

Stepper a(2, 3);
Stepper b(4, 5);
StepperGroup ab{a, b};

// What the motors can do: maximum speed (steps/s), acceleration (steps/s^2), and the largest
// change of speed without a ramp (steps/s)
const planner::MotorLimits limits[2] = {
    {20'000, 50'000, 500}, // A
    {20'000, 50'000, 500}, // B
};

// 80 steps of a motor move the head by 1 mm along x or y
const planner::Kinematics machine = planner::Kinematics::coreXY(80, 80);

planner::Segment segments[100]; // room for the path
planner::Path path(segments, 100, machine, limits);
planner::PathRunner runner(ab);

float factor = 1;  // of the planned speeds
bool held    = false;

void makePath()
{
    const float feed = 80; // mm/s

    path.start({0, 0}); // where the head stands
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

void readCommands()
{
    if (!Serial.available()) return;
    char c = Serial.read();
    if (c == '+') factor = min(factor + 0.1f, 1.0f);
    if (c == '-') factor = max(factor - 0.1f, 0.1f);
    if (c == 'h') held = true;
    if (c == 'g') held = false;
    if (c != '+' && c != '-' && c != 'h' && c != 'g') return;

    runner.moves.overrideSpeed(held ? 0 : factor); // 0 holds the path; the factor stays for the paths that follow
    Serial.printf("%s, %.0f %% of the planned speeds\n", held ? "held" : "running", factor * 100);
}

void setup()
{
    a.setMaxSpeed(limits[0].maxSpeed).setAcceleration(limits[0].acceleration); // the same limits as the planner has
    b.setMaxSpeed(limits[1].maxSpeed).setAcceleration(limits[1].acceleration);

    makePath();
    runner.start(path);
}

void loop()
{
    runner.giveMoves(); // also while the path is held: the head brakes along the moves that follow
    readCommands();

    if (!runner.isRunning()) // the path is done: again after a second
    {
        delay(1000);
        runner.start(path);
    }
}
