#pragma once

#include "Arduino.h"
#include <functional>

#include "../TF_Handler.h"

// //=========================
// // ESP 32
// //=========================

class TimerField
{
public:
  inline TimerField(TF_Handler *_handler);

  inline bool begin();
  inline void end();

  inline void stepTimerStart();
  inline void stepTimerStop();
  inline void setStepFrequency(unsigned f);
  inline unsigned getStepFrequency() { return timerAlarmRead(stepTimer); }
  inline bool stepTimerIsRunning() const { return stepTimerRunning; }

  inline void accTimerStart() { timerAlarmEnable(accTimer); }
  inline void accTimerStop() { timerAlarmDisable(accTimer); }
  inline void setAccUpdatePeriod(unsigned period) { timerAlarmWrite(accTimer, period, true); }

  inline void triggerDelay() { timerAlarmEnable(pulseTimer); }
  inline void setPulseWidth(unsigned pulseWidth) { timerAlarmWrite(pulseTimer, pulseWidth, false); }

  static portMUX_TYPE timerMux;

protected:
  static TF_Handler *handler;
  hw_timer_t *stepTimer;
  hw_timer_t *accTimer;
  hw_timer_t *pulseTimer;
  bool stepTimerRunning;
  unsigned lastF;
};

// IMPLEMENTATION ====================================================================

TimerField::TimerField(TF_Handler *_handler)
    : stepTimer(timerBegin(0, 80, true)),
      accTimer(timerBegin(1, 80, true)),
      pulseTimer(timerBegin(2, 80, true)),
      stepTimerRunning(false),
      lastF(0)
{
  handler = _handler;
  timerAttachInterrupt(stepTimer, [] { handler->stepTimerISR(); }, true);
  timerAlarmWrite(stepTimer, 1, true);
  timerAttachInterrupt(accTimer, [] { handler->accTimerISR(); }, true);
  timerAlarmWrite(accTimer, 1, true);
  timerAttachInterrupt(pulseTimer, [] { handler->pulseTimerISR(); }, true);
  timerAlarmWrite(pulseTimer, 1, false);
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

void TimerField::setStepFrequency(unsigned f)
{
  if (f != 0)
    timerAlarmWrite(stepTimer, 1000000 / f, true); // Timer runs at 1 microsecond interval
  else
    stepTimerStop();
}

bool TimerField::begin()
{
  Serial.println("begin");
  return true;
}

void TimerField::end()
{
  timerAlarmDisable(stepTimer);
  timerAlarmDisable(accTimer);
  timerAlarmDisable(pulseTimer);
}