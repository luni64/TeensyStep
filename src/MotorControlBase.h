#pragma once


#include "./timer/teensy3/TeensyDelay/TeensyDelay.h"

class Stepper;

constexpr int MaxMotors = 10;

class MotorControlBase 
{
  public:
    inline bool isOk() const { return OK; } 

  protected:
    MotorControlBase()
        // : pinResetDelayChannel(TeensyDelay::addDelayChannel(this)),
        //   accLoopDelayChannel(TeensyDelay::addDelayChannel(this))
    {
        mCnt = 0;        
       // TeensyDelay::begin();        
    }

    //virtual void pitISR() = 0;
    //virtual void delayISR(unsigned channel) = 0;
    
    bool OK = false; 

    // const unsigned pinResetDelayChannel;
    // const unsigned accLoopDelayChannel;
    Stepper *motorList[MaxMotors+1];
    Stepper *leadMotor;
    unsigned mCnt;
};
