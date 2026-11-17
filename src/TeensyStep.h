#pragma once

// TeensyStep: stepper motors on drivers with STEP and DIR inputs.
//
//   #include "TeensyStep.h"
//   using namespace TeensyStep;
//
//   Stepper motor(2, 3);          // STEP pin 2, DIR pin 3
//   motor.moveRel(1000);
//
// Stepper: one motor. StepperGroup: motors that move synchronized. errorCode, attachErrFunc, ErrorHandler: errors.

#include "api/ErrorHandler.h"
#include "api/Legacy.h"
#include "api/Stepper.h"
#include "api/StepperGroup.h"
#include "core/errorCode.h"

// Sketches written for TeensyStep 2 use these names without a namespace. Define TEENSYSTEP_NO_GLOBAL_NAMES
// before the include to switch them off, e.g. if another library has a class named Stepper.
#if !defined(TEENSYSTEP_NO_GLOBAL_NAMES)
using Stepper = TeensyStep::Stepper;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
using StepControl [[deprecated("TeensyStep 2 controller: use the move commands of Stepper and StepperGroup, e.g. motor.moveAbs(target)")]]     = TeensyStep::StepControl;
using RotateControl [[deprecated("TeensyStep 2 controller: use rotateAsync() of Stepper and StepperGroup, e.g. motor.rotateAsync(speed)")]] = TeensyStep::RotateControl;
#pragma GCC diagnostic pop
#endif
