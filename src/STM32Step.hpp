#pragma once

#include "version.h"

#include "step_control/GPIOStepControl.hpp"

#include "PositionControlBase.hpp"
//#include "SpeedControlBase.hpp"

#include "accelerators/LinPositionAccelerator.h"
#include "accelerators/LinSpeedAccelerator.h"
//#include "accelerators/SinSpeedAccelerator.h"

using PositionControl = PositionControlBase<LinPositionAccelerator, GPIOStepControl>;
//using SpeedControl = SpeedControlBase<LinSpeedAccelerator, GPIOStepControl>;

