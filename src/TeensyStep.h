#pragma once

#include "RotateControlBase.h"
#include "StepControlBase.h"

#include "Accelerators/LinRotAccelerator.h"
#include "Accelerators/LinStepAccelerator.h"

#include "Accelerators/SinRotAccelerator.h"

constexpr unsigned defPW = 5;
constexpr unsigned defAP = 5000;

// TEENSY ==========================================================================================
#if defined TEENSYDUINO

// Linear acceleration --------------

template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
using RotateControl = RotateControlBase<LinRotAccelerator, stepPulseWidth, accUpdatePeriod>;

template <unsigned stepPulseWidth = defPW, unsigned a = defAP>
using StepControl = StepControlBase<LinStepAccelerator, stepPulseWidth, a>;


// Sine acceleration ---------------

template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
using RotateControlSin = RotateControlBase<SinRotAccelerator, stepPulseWidth, accUpdatePeriod>;

//template <unsigned p = defPW, unsigned a>
//using StepControlSin = StepControlBase<SinStepAccelerator, p, a>;

//STM32 =============================================================================================
#elif defined STM32

template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
using RotateControl = RotateControlBase<LinRotAccelerator, stepPulseWidth, accUpdatePeriod>;

template <unsigned stepPulseWidth = defPW, unsigned a = defAP>
using StepControl = RotateControlBase<LinStepAccelerator, stepPulseWidth, a>;

#endif

// Generic ==========================================================================================

template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
using RotateControl_tick = RotateControlBase<LinRotAccelerator, stepPulseWidth, accUpdatePeriod>;

template <unsigned stepPulseWidth = defPW, unsigned a = defAP>
using StepControl_tick = StepControlBase<LinStepAccelerator, stepPulseWidth, a>;
