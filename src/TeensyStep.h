#pragma once

#include "RotateControlBase.h"
#include "StepControlBase.h"

#include "Accelerators/LinRotAccelerator.h"
#include "Accelerators/SinRotAccelerator.h"

// TEENSY ==========================================================================================

#if defined TEENSYDUINO

template <unsigned stepPulseWidth = 5, unsigned accUpdatePeriod = 5000>
using RotateControl = RotateControlBase<LinRotAccelerator, stepPulseWidth, accUpdatePeriod>;

template <unsigned stepPulseWidth = 5, unsigned accUpdatePeriod = 5000>
using RotateControlSin = RotateControlBase<SinRotAccelerator, stepPulseWidth, accUpdatePeriod>;

//template <unsigned p, unsigned a>
//using StepControl = RotateControlBase<LinRotAccelerator, p, a>;

//STM32 =============================================================================================

#elif defined STM32

// template <unsigned p, unsigned a>
// using RotateControl = RotateControlBase<LinRotAccelerator, p, a>;

#endif

// Generic ==========================================================================================

template <unsigned stepPulseWidth = 5, unsigned accUpdatePeriod = 5000>
using RotateControlTick = RotateControlBase<LinRotAccelerator, stepPulseWidth, accUpdatePeriod>;  

