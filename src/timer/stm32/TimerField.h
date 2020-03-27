#pragma once

#include <Arduino.h>
#include <functional>

#include "../TF_Handler.h"

class TimerField
{
public:
  inline TimerField(TF_Handler *_handler);

  inline bool begin();
  inline void end();

  inline void stepTimerStart();
  inline void stepTimerStop();
  inline void setStepFrequency(unsigned f);
  inline unsigned getStepFrequency() { return stepTimer.getTimerClkFreq(); } //  clock of timer  or overflow?
  inline bool stepTimerIsRunning() const { return stepTimerRunning; }

  inline void accTimerStart() { accTimer.resume(); }
  inline void accTimerStop() { accTimer.pause(); }
  inline void setAccUpdatePeriod(unsigned period) { accTimer.setOverflow(period, MICROSEC_FORMAT); } //timerAlarmWrite(accTimer, period, true); }

  inline void triggerDelay() { pulseTimer.resume(); }
  inline void setPulseWidth(unsigned pulseWidth) { pulseTimer.setOverflow(pulseWidth, MICROSEC_FORMAT); } // pause/stop or configure on the go works?

protected:
  static TF_Handler *handler;
  HardwareTimer stepTimer;
  HardwareTimer accTimer;
  HardwareTimer pulseTimer;
  volatile bool stepTimerRunning;
  volatile unsigned lastF;
};

// IMPLEMENTATION ====================================================================

TimerField::TimerField(TF_Handler *_handler) :
      stepTimer(TIM7),
      accTimer(TIM8),
      pulseTimer(TIM9),
      stepTimerRunning(false),
      lastF(0)
{
  handler = _handler;
  stepTimer.attachInterrupt([] { handler->stepTimerISR(); });
  accTimer.attachInterrupt([] { handler->accTimerISR(); });
  pulseTimer.attachInterrupt([] { handler->pulseTimerISR(); });
}

void TimerField::stepTimerStart()
{
  stepTimer.resume();
  stepTimerRunning = true;
}

void TimerField::stepTimerStop()
{
  stepTimer.pause();
  stepTimerRunning = false;
}

void TimerField::setStepFrequency(unsigned f)
{
  if(f == 0){
    stepTimerStop();
    return;
  }
  stepTimer.setOverflow(f, HERTZ_FORMAT);
 // stepTimer.refresh();
    /*
    m_timer.pause();
    m_timer.setOverflow(micros, MICROSEC_FORMAT);
    m_timer.refresh();
    m_timer.resume();
    */
}

bool TimerField::begin()
{
  Serial.println("begin");
  return true;
}

void TimerField::end()
{
  stepTimer.pause();
  accTimer.pause();
  pulseTimer.pause();
}