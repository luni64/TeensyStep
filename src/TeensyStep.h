#pragma once

#include "RotateControlBase.h"
#include "StepControlBase.h"

#include "Accelerators/LinRotAccelerator.h"
#include "Accelerators/LinStepAccelerator.h"

#include "Accelerators/SinRotAccelerator.h"

constexpr unsigned defPW = 5;
constexpr unsigned defAP = 5000;

// TEENSY 3.0 - Teensy 3.6 ==================================================================================

#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
#include "timer/teensy3/TimerField.h"

// TEENSY 4 ================================================================================================

#elif defined(__TEENSY4_TBD__)
#include "timer/teensy4/TimerField.h"

//STM32 ====================================================================================================

#elif defined(__STM32_TBD__)
#include "timers/STM32/TimerField.h"

//Some other hardware ======================================================================================

#elif defined(__someHardware_TBD__)
#include "timers/someHardware/TimerField.h"
#endif

// Linear acceleration -----------------------------------------------------------------------------------------

template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
using RotateControl = RotateControlBase<LinRotAccelerator, TimerField, stepPulseWidth, accUpdatePeriod>;

template <unsigned stepPulseWidth = defPW, unsigned a = defAP>
using StepControl = StepControlBase<LinStepAccelerator, TimerField, stepPulseWidth, a>;

// Sine acceleration -------------------------------------------------------------------------------------------

template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
using RotateControlSin = RotateControlBase<SinRotAccelerator, TimerField, stepPulseWidth, accUpdatePeriod>;

//template <unsigned p = defPW, unsigned a>
//using StepControlSin = StepControlBase<SinStepAccelerator, p, a>;

// Generic ==========================================================================================

// template <unsigned stepPulseWidth = defPW, unsigned accUpdatePeriod = defAP>
// using RotateControl_tick = RotateControlBase<LinRotAccelerator, stepPulseWidth, accUpdatePeriod>;

// template <unsigned stepPulseWidth = defPW, unsigned a = defAP>
// using StepControl_tick = StepControlBase<LinStepAccelerator, stepPulseWidth, a>;
