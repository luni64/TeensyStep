#pragma once

#include "TickTimer.h"
#include "../TF_Handler.h"

class TickTimerField
{
  public:
    inline TickTimerField(TF_Handler *);

    inline bool begin(TIM_HandleTypeDef* htim);
    inline void end();

    inline void stepTimerStart() { stepTimer.start(); }
    inline void stepTimerStop() { stepTimer.stop(); }
    inline bool stepTimerIsRunning() const { return stepTimer.isRunning(); }
    inline void setStepFrequency(unsigned f) {stepTimer.setFrequency(f); }
    inline unsigned getStepFrequency() { return 0; }

    inline void accTimerStart() { accTimer.start(); }
    inline void accTimerStop() { accTimer.stop(); }
    inline void setAccUpdatePeriod(unsigned period) { accTimer.setPeriod(period); }

    inline void setPulseWidth(unsigned delay) { delayTimer.setDelay(delay); }
    inline void triggerDelay() { delayTimer.start(); }

  protected:
    TF_Handler *handler;

    PeriodicTimer stepTimer, accTimer;
    OneShotTimer delayTimer;
};

// IMPLEMENTATION ====================================================================

TickTimerField::TickTimerField(TF_Handler *_handler)
    : handler(_handler),
      stepTimer([this] { handler->stepTimerISR(); }),
      //stepTimer(test),
      accTimer([_handler] { _handler->accTimerISR(); }),
      delayTimer([this] { handler->pulseTimerISR(); })
{
    TimerControl::attachTimer(&stepTimer);
    TimerControl::attachTimer(&accTimer);
    TimerControl::attachTimer(&delayTimer);
}



bool TickTimerField::begin(TIM_HandleTypeDef* htim)
{
    TimerControl::begin(htim);
    return true;
}

void TickTimerField::end()
{
    //TimerControl::end();    
}