#pragma once

#include "PIT.h"

struct TF_Handler
{
  virtual void stepTimerISR() = 0;
  virtual void accTimerISR() = 0;
  virtual void delayTimerISR() = 0;
};

//=========================
// Teensy3
//=========================

class TimerField
{
public:
  inline bool begin(TF_Handler *);
  inline void end();

  inline void stepTimerStart();
  inline void stepTimerStop();
  inline bool stepTimerIsRunning() const;
  inline void stepTimerSetFrequency(unsigned f);

  inline void accTimerStart();
  inline void accTimerStop();  
  inline void accTimerSetFrequency(unsigned f);

  inline void delayTimerTrigger(unsigned delay);

protected:
  PIT stepTimer;
  TF_Handler *handler;
};

// IMPLEMENTATION ----------------------------------------------------------------

bool TimerField::begin(TF_Handler *handler)
{
  this->handler = handler;
  return stepTimer.begin(handler);
}

void TimerField::end()
{
  stepTimer.end();
}

void TimerField::stepTimerStart() 
{
  stepTimer.start();
}

void TimerField::stepTimerStop()
{
  stepTimer.stop();
}

void TimerField::stepTimerSetFrequency(unsigned f)
{
  stepTimer.setFrequency(f);
}

bool TimerField::stepTimerIsRunning() const
{
  return stepTimer.channel->TCTRL & PIT_TCTRL_TIE;
}
