#pragma once

#include <functional>
#include "TeensyStep.h"

// template<typename T>
// class FlexMotorControlBase<T>;

using PathFunction = std::function<float(float)>;

class PathStepper : public Stepper
{
public:
  using Stepper::Stepper;

  void attachFunction(PathFunction f) { pathFunction = f; }
  void setScale(float s) { scaleFactor = s; }

protected:
  float scaleFactor = 1.0f;
  PathFunction pathFunction;

  template <typename t>
  friend class PathControlBase;
};