#pragma once

#include "version.h"

#include "RotateControlBase.h"
#include "StepControlBase.h"

#include "accelerators/LinRotAccelerator.h"
#include "accelerators/LinStepAccelerator.h"
//#include "accelerators/SinRotAccelerator.h"

#include "timer/generic/TimerField.h"

using TimerField = TickTimerField;

// Linear acceleration -----------------------------------------------------------------------------------------


using MotorControl = MotorControlBase<TimerField>;

using RotateControl = RotateControlBase<LinRotAccelerator, TimerField>;
using StepControl = StepControlBase<LinStepAccelerator, TimerField>;

using StepControlTick = StepControlBase<LinStepAccelerator,TickTimerField>;
using RotateControlTick = RotateControlBase<LinStepAccelerator,TickTimerField>;

// Sine acceleration -------------------------------------------------------------------------------------------

// template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
// using RotateControlSin = RotateControlBase<SinRotAccelerator, TimerField>;

//template <unsigned p = defPW, unsigned a>
//using StepControlSin = StepControlBase<SinStepAccelerator, p, a>;

// Generic ==========================================================================================

// template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
// using RotateControl_tick = RotateControlBase<LinRotAccelerator, stepPulseWidth, accUpdatePeriod>;

// template <unsigned stepPulseWidth = defPW, unsigned a = defAP>
// using StepControl_tick = StepControlBase<LinStepAccelerator, stepPulseWidth, a>;
