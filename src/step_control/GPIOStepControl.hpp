#pragma once

#include "stepper/GPIOStepper.hpp"
#include "timer/GPIOStepTimeControl.hpp"

constexpr int MaxMotors = 10;

/**
 * Motor controllers manage the synchronous movements of stepper motors
 * by direct calls to steppers and using timer callbacks.
 * */
class GPIOStepControl
{
public:
  using Stepper = GPIOStepper;

  void begin();
  bool isRunning();
  inline int getCurrentSpeed()
  {
    return stepTimeControl.getStepFrequency();
  }

  void emergencyStop() { stepTimeControl.stepTimerStop();}

  ~GPIOStepControl();
  bool isOk() const { return OK; }

protected:

  GPIOStepControl(TimerArrayControl& control, uint32_t pulseWidth, uint32_t accUpdatePeriod, Timer* _accTimer);

  template <size_t N>
  void attachStepper(Stepper *(&motors)[N]);
  template <typename... Steppers>
  void attachStepper(Stepper &stepper, Steppers &... steppers);
  void attachStepper(Stepper &stepper);

  static void stepTimerISR(GPIOStepControl* ctx);
  static void pulseTimerISR(GPIOStepControl* ctx);

  Stepper *motorList[MaxMotors + 1];
  Stepper *leadMotor;

  void (*callback)() = nullptr;

  bool OK = false;

  unsigned mCnt;

  enum class Mode
  {
    target,
    notarget
  } mode = Mode::notarget;

  ContextTimer<GPIOStepControl> stepTimer, delayTimer;
  GPIOStepTimeControl stepTimeControl;
  uint32_t accUpdatePeriod, pulseWidth;

  GPIOStepControl(const GPIOStepControl &) = delete;
  GPIOStepControl &operator=(const GPIOStepControl &) = delete;

  template <typename Accelerator, typename MotorControl>
  friend class PositionControlBase;
};

// Implementation ============================================================================

bool GPIOStepControl::isRunning()
{
  return stepTimeControl.stepTimerIsRunning();
}

GPIOStepControl::GPIOStepControl(TimerArrayControl& control, uint32_t _pulseWidth, uint32_t _accUpdatePeriod, Timer* _accTimer)
    : mCnt(0),
    stepTimer(0, true, this, stepTimerISR),
    delayTimer(0, false, this, pulseTimerISR),
    stepTimeControl(control, &stepTimer, &delayTimer, _accTimer),
    accUpdatePeriod(_accUpdatePeriod),
    pulseWidth(_pulseWidth)
{}

void GPIOStepControl::begin(){
  stepTimeControl.setPulseWidth(pulseWidth);
  stepTimeControl.setAccUpdatePeriod(accUpdatePeriod);
  OK = stepTimeControl.begin();
}
 
GPIOStepControl::~GPIOStepControl()
{
  if(OK)  emergencyStop();  
}

void GPIOStepControl::stepTimerISR(GPIOStepControl* ctx)
{
  ctx->leadMotor->doStep(); // move master motor

  Stepper **slave = ctx->motorList;
  while (*(++slave) != nullptr) // move slave motors if required (https://en.wikipedia.org/wiki/Bresenham)
  {
    if ((*slave)->B >= 0)
    {
      (*slave)->doStep();
      (*slave)->B -= ctx->leadMotor->A;
    }
    (*slave)->B += (*slave)->A;
  }
  ctx->stepTimeControl.triggerDelay(); // start delay line to dactivate all step pins

  if (ctx->mode == Mode::target && (ctx->leadMotor->current == ctx->leadMotor->target)) // stop timer and call callback if we reached target
  {
    ctx->stepTimeControl.stepTimerStop();
    if (ctx->callback != nullptr)
      ctx->callback();
  }
}

void GPIOStepControl::pulseTimerISR(GPIOStepControl* ctx)
{
  Stepper **motor = ctx->motorList;
  while ((*motor) != nullptr)
  {
    (*motor++)->clearStepPin();
  }
}

void GPIOStepControl::attachStepper(Stepper &stepper)
{
  motorList[mCnt++] = &stepper;
  motorList[mCnt] = nullptr;
  mCnt = 0;
}

template <typename... Steppers>
void GPIOStepControl::attachStepper(Stepper &stepper, Steppers &... steppers)
{
  static_assert(sizeof...(steppers) < MaxMotors, "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

  motorList[this->mCnt++] = &stepper;
  attachStepper(steppers...);
}

template <size_t N>
void GPIOStepControl::attachStepper(Stepper *(&motors)[N])
{
  static_assert(N <= MaxMotors, "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

  for (size_t i = 0; i < N; i++)
  {
    this->motorList[i] = motors[i];
  }
  this->motorList[N] = nullptr;
}