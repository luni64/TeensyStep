#pragma once

#include "Arduino.h"
#include <functional>
// //#include "imxrt.h"

#include "../TF_Handler.h"

// //=========================
// // ESP 32
// //=========================

//

class TimerField
{
public:
  inline TimerField(TF_Handler *_handler);

  inline bool begin();
  inline void end();

  inline void stepTimerStart();
  inline void stepTimerStop();
  inline bool stepTimerIsRunning() const { return stepTimerRunning; }
  inline void setStepFrequency(unsigned f) { timerAlarmWrite(stepTimer, 1. / f, true); }
  inline unsigned getStepFrequency() { return 0; }

  inline void accTimerStart() { timerAlarmEnable(accTimer); }
  inline void accTimerStop() { timerAlarmDisable(accTimer); }
  inline void setAccUpdatePeriod(unsigned period) { timerAlarmWrite(accTimer, period, true); }

  inline void setPulseWidth(unsigned delay) { timerAlarmWrite(stepTimer, delay, false); }
  inline void triggerDelay() { timerAlarmEnable(delayTimer); }

  void setupStepTimer(void (*cb)(void));
  void setupAccTimer(void (*cb)(void));
  void setupDelayTimer(void (*cb)(void));
protected:
  TF_Handler *handler;
  hw_timer_t *stepTimer;
  hw_timer_t *accTimer;
  hw_timer_t *delayTimer;
  bool stepTimerRunning;
};

// IMPLEMENTATION ====================================================================

TimerField::TimerField(TF_Handler *_handler)
    : handler(_handler),
      stepTimer(timerBegin(0, 80, true)),
      accTimer(timerBegin(1, 80, true)),
      delayTimer(timerBegin(2, 80, true)),
      stepTimerRunning(false)
{
}

void TimerField::setupStepTimer(void (*cb)(void))
{
  timerAttachInterrupt(stepTimer, cb, true);
  timerAlarmWrite(stepTimer, 1, true);
}

void TimerField::setupAccTimer(void (*cb)(void))
{
  timerAttachInterrupt(accTimer, cb, true);
  timerAlarmWrite(accTimer, 1, true);
}

void TimerField::setupDelayTimer(void (*cb)(void))
{
  timerAttachInterrupt(delayTimer, cb, true);
  timerAlarmWrite(delayTimer, 1, false);
}

void TimerField::stepTimerStart()
{
  timerAlarmEnable(stepTimer);
  stepTimerRunning = true;
}

void TimerField::stepTimerStop()
{
  timerAlarmDisable(stepTimer);
  stepTimerRunning = false;
}

bool TimerField::begin()
{
  Serial.println("begin");
  return true;
}

void TimerField::end()
{
  //TimerControl::end();
}