#pragma once

#include "wiring.h"
// //#include "imxrt.h"

#include "TickTimer.h"
#include "../TF_Handler.h"

// //=========================
// // Teensy 4
// //=========================

class TickTimerField
{
  public:
    inline TickTimerField(TeensyStep::TF_Handler *);

    inline bool begin();
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
    TeensyStep::TF_Handler *handler;

    PeriodicTimer stepTimer, accTimer;
    OneShotTimer delayTimer;
};

// IMPLEMENTATION ====================================================================

TickTimerField::TickTimerField(TeensyStep::TF_Handler *_handler)
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



bool TickTimerField::begin()
{
    Serial.println("begin");
    TimerControl::begin();
    return true;
}

void TickTimerField::end()
{
    //TimerControl::end();    
}