#pragma once

#include "./timer/teensy3/PIT.h"
#include "./TeensyDelay/TeensyDelay.h"

class Stepper;

constexpr int MaxMotors = 10;

class MotorControlBase : IDelayHandler
{
  public:
    inline bool isOk() const { return OK; } 
    //inline bool isRunning() const { return StepTimer.channel->TCTRL & PIT_TCTRL_TIE; }
    //inline void emergencyStop() const {StepTimer.stop(); }
    // inline void const stop()
    // {
    //     stopAsync();
    //     while (isRunning())
    //     {
    //         delay(10);
    //     }
    // }

  protected:
    MotorControlBase()
        : pinResetDelayChannel(TeensyDelay::addDelayChannel(this)),
          accLoopDelayChannel(TeensyDelay::addDelayChannel(this))
    {
        mCnt = 0; 
       // OK = StepTimer.begin(this);
        TeensyDelay::begin();        
    }

    //virtual void pitISR() = 0;
    virtual void delayISR(unsigned channel) = 0;
    virtual void stopAsync() = 0;

    bool OK = false;
   // PIT StepTimer;

    const unsigned pinResetDelayChannel;
    const unsigned accLoopDelayChannel;
    Stepper *motorList[MaxMotors+1];
    Stepper *leadMotor;
    unsigned mCnt;
};
