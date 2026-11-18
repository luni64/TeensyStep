#pragma once

// TeensyStep, advanced functions: for programs that calculate the motion themselves, for example a motion
// planner. They are not needed to move motors and groups; for that, include TeensyStep.h.
//
//   #include "TeensyStepAdvanced.h"
//   using namespace TeensyStep;
//
//   StepperGroup xy{x, y};
//   advanced::GroupPath path(xy);
//   path.moveAbsAsync({10000, 0}, 500);   // pass the targets at 500 steps/s; the next move follows without a stop
//
// advanced::GroupPath: moves of a group that follow each other without a stop.

#include "TeensyStep.h"
#include "api/GroupPath.h"
