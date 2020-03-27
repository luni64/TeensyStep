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

  inline void accTimerStart() { accTimer.resume(); accTimer.refresh(); }
  inline void accTimerStop() { accTimer.pause(); }
  inline void setAccUpdatePeriod(unsigned period);

  inline void triggerDelay();
  inline void setPulseWidth(unsigned pulseWidth);

protected:
  static TF_Handler *handler;
  HardwareTimer stepTimer;
  HardwareTimer accTimer;
  HardwareTimer pulseTimer;
  volatile bool stepTimerRunning;
};

// IMPLEMENTATION ====================================================================

TimerField::TimerField(TF_Handler *_handler) :
      stepTimer(TIM7),
      accTimer(TIM8),
      pulseTimer(TIM9),
      stepTimerRunning(false)
{
  handler = _handler;
  stepTimer.attachInterrupt([] { handler->stepTimerISR(); });
  accTimer.attachInterrupt([] { handler->accTimerISR(); });
  pulseTimer.attachInterrupt([this] { handler->pulseTimerISR(); this->pulseTimer.pause(); }); // one-shot mode
}

void TimerField::stepTimerStart()
{
  stepTimer.resume();
  // force reload of all timer's registers here saves 1.2ms in startup time
  // https://github.com/stm32duino/wiki/wiki/HardwareTimer-library
  stepTimer.refresh();
  stepTimerRunning = true;
}

void TimerField::stepTimerStop()
{
  stepTimer.pause();
  stepTimerRunning = false;
}

void TimerField::setAccUpdatePeriod(unsigned period)
{
  accTimer.setOverflow(period, MICROSEC_FORMAT);
}

void TimerField::triggerDelay() {
  pulseTimer.resume();
}

void TimerField::setPulseWidth(unsigned pulseWidth)
{
  pulseTimer.setOverflow(pulseWidth, MICROSEC_FORMAT);
}

void TimerField::setStepFrequency(unsigned f)
{
  if(f == 0){
    stepTimerStop();
    return;
  }
  stepTimer.setOverflow(f, HERTZ_FORMAT);
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