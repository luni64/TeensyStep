#pragma once

#include "timer/TF_Handler.h"
#include "Stepper.h"

constexpr int MaxMotors = 10;

template <typename TimerField>
class MotorControlBase : TF_Handler
{
public:
  bool isRunning();
  inline int getCurrentSpeed()
  {
    return timerField.getStepFrequency();
  }

  void emergencyStop() { timerField.stepTimerStop();}

  virtual ~MotorControlBase();
  bool isOk() const { return OK; }

protected:
  TimerField timerField;
  MotorControlBase(unsigned pulseWidth, unsigned accUpdatePeriod);

  template <size_t N>
  void attachStepper(Stepper *(&motors)[N]);
  template <typename... Steppers>
  void attachStepper(Stepper &stepper, Steppers &... steppers);
  void attachStepper(Stepper &stepper);

  void stepTimerISR();
  //void accTimerISR() { Serial.println("df"); }
  void pulseTimerISR();

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

  uint32_t accUpdatePeriod;
  uint32_t pulseWidth;

  MotorControlBase(const MotorControlBase &) = delete;
  MotorControlBase &operator=(const MotorControlBase &) = delete;
};

// Implementation ============================================================================

template <typename t>
bool MotorControlBase<t>::isRunning()
{
  return timerField.stepTimerIsRunning();
}

template <typename t>
MotorControlBase<t>::MotorControlBase(unsigned pulseWidth, unsigned accUpdatePeriod)
    : timerField(this), mCnt(0)
{
  OK = timerField.begin();
  timerField.setPulseWidth(pulseWidth);
  timerField.setAccUpdatePeriod(accUpdatePeriod);
  this->accUpdatePeriod = accUpdatePeriod;
  this->pulseWidth = pulseWidth;
}
 
template <typename t>
MotorControlBase<t>::~MotorControlBase()
{
  if(OK)  emergencyStop();  
}

template <typename t>
void MotorControlBase<t>::stepTimerISR()
{
  leadMotor->doStep(); // move master motor

  Stepper **slave = motorList;
  while (*(++slave) != nullptr) // move slave motors if required (https://en.wikipedia.org/wiki/Bresenham)
  {
    if ((*slave)->B >= 0)
    {
      (*slave)->doStep();
      (*slave)->B -= leadMotor->A;
    }
    (*slave)->B += (*slave)->A;
  }
  timerField.triggerDelay(); // start delay line to dactivate all step pins

  if (mode == Mode::target && (leadMotor->current == leadMotor->target)) // stop timer and call callback if we reached target
  {
    timerField.stepTimerStop();
    if (callback != nullptr)
      callback();
  }
}

template <typename t>
void MotorControlBase<t>::pulseTimerISR()
{
  Stepper **motor = motorList;
  while ((*motor) != nullptr)
  {
    (*motor++)->clearStepPin();
  }
}

template <typename t>
void MotorControlBase<t>::attachStepper(Stepper &stepper)
{
  motorList[mCnt++] = &stepper;
  motorList[mCnt] = nullptr;
  mCnt = 0;
}

template <typename t>
template <typename... Steppers>
void MotorControlBase<t>::attachStepper(Stepper &stepper, Steppers &... steppers)
{
  static_assert(sizeof...(steppers) < MaxMotors, "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

  motorList[this->mCnt++] = &stepper;
  attachStepper(steppers...);
}

template <typename t>
template <size_t N>
void MotorControlBase<t>::attachStepper(Stepper *(&motors)[N])
{
  static_assert(N <= MaxMotors, "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

  for (size_t i = 0; i < N; i++)
  {
    this->motorList[i] = motors[i];
  }
  this->motorList[N] = nullptr;
}