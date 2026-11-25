#pragma once

// TeensyStep, advanced functions: for programs that calculate the motion themselves, for example a motion
// planner. They are not needed to move motors and groups; for that, include TeensyStep.h.
//
//   #include "TeensyStepAdvanced.h"
//   using namespace TeensyStep;
//
//   StepperGroup xy{x, y};
//   advanced::GroupPath path(xy);
//   path.add({10000, 0},    {100, 50, 10});   // 100 mm at 50 mm/s, pass the targets at 10 mm/s
//   path.add({10000, 5000}, {50, 50, 0});     // the next move, given ahead: it follows without a stop
//
// advanced::GroupPath: moves of a group that follow each other without a stop, given ahead, with their
//                      speeds in the unit of the caller.
// advanced::setInterruptPriority(): priority of the interrupts in which the steps are made.

#include "TeensyStep.h"
#include "api/GroupPath.h"

namespace TeensyStep
{
    namespace advanced
    {
        /// Priority of the step interrupts unless setInterruptPriority() is called.
        constexpr uint8_t defaultInterruptPriority = backend::defaultInterruptPriority;

        /// @brief Sets the priority of the interrupts in which the step pulses are made. The numbers are
        /// those of the Teensy core (NVIC_SET_PRIORITY): 0 is the most urgent and 255 the least, in steps
        /// of 16; the default of the core for other interrupts is 128, its millisecond interrupt has 32.
        ///
        /// Default: 48. Interrupts with a lower number than the step interrupts delay the step pulses
        /// while they run; interrupts with a higher number are delayed by the step interrupts. With a
        /// priority of 32 or less, an arrival function that takes long stops millis().
        ///
        /// Takes effect at once, also for motors that are moving.
        inline void setInterruptPriority(uint8_t priority) { backend::setInterruptPriority(priority); }
    }
}
