#pragma once

#include "./PIT/PIT.h"
#include "./TeensyDelay/TeensyDelay.h"
//#include "Stepper.h"
class Stepper;

constexpr int MaxMotors = 10;

template <unsigned pulseWidth, unsigned accUpdatePeriod>
class MotorControlBase : IPitHandler, IDelayHandler
{
  public:
    bool isOk() { return OK; }
    bool isRunning() { return StepTimer.channel->TCTRL & PIT_TCTRL_TIE; }
    void emergencyStop() { this->StepTimer.disableInterupt(); }
    void stop();

  protected:
    MotorControlBase();
    
    virtual void pitISR() = 0;
    virtual void delayISR(unsigned channel) = 0;
    void virtual stopAsync() = 0;

    bool OK = false;
    PIT StepTimer;

    const unsigned pinResetDelayChannel;
    const unsigned accLoopDelayChannel;
    Stepper *motorList[MaxMotors];
};

// Implementation *************************************************************************************************

template <unsigned p, unsigned u>
MotorControlBase<p, u>::MotorControlBase() : pinResetDelayChannel(TeensyDelay::addDelayChannel(this)), accLoopDelayChannel(TeensyDelay::addDelayChannel(this))
{
    OK = StepTimer.begin(this);
    TeensyDelay::begin();
}

template <unsigned p, unsigned u>
void MotorControlBase<p, u>::stop()
{
    stopAsync();
    while (isRunning())
    {
        delay(10);
    }
}