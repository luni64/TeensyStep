#pragma once

#include "wiring.h"
#include "PIT.h"
#include "TeensyStepFTM.h"

#include "../TF_Handler.h"
//#include "../TimerFieldBase.h"

//=========================
// Teensy3
//=========================

class TimerField : public IDelayHandler
{
public:
  inline TimerField(TF_Handler *);

  inline bool begin();
  inline void end();

  inline void stepTimerStart();
  inline void stepTimerStop();
  inline bool stepTimerIsRunning() const;
  inline void setStepFrequency(unsigned f);

  inline void accTimerStart();
  inline void accTimerStop();
  inline void setAccUpdatePeriod(unsigned period);

  inline void setPulseWidth(unsigned delay);
  inline void triggerDelay();
  inline void delayISR(unsigned channel);

protected:
  PIT stepTimer;
  TF_Handler *handler;

  unsigned delayWidth;
  unsigned accUpdatePeriod;

  const unsigned accLoopDelayChannel;
  const unsigned pinResetDelayChannel;
};

// IMPLEMENTATION ====================================================================

TimerField::TimerField(TF_Handler *_handler)
    : handler(_handler),
      accLoopDelayChannel(TeensyStepFTM::addDelayChannel(this)),
      pinResetDelayChannel(TeensyStepFTM::addDelayChannel(this))
{
  //Serial.println(accLoopDelayChannel);
}

bool TimerField::begin()
{
  TeensyStepFTM::begin();
  return stepTimer.begin(handler);
}

void TimerField::end()
{
  stepTimer.end();
  TeensyStepFTM::removeDelayChannel(accLoopDelayChannel);
  TeensyStepFTM::removeDelayChannel(pinResetDelayChannel);
}

// Step Timer ------------------------------------------------------

void TimerField::stepTimerStart()
{
  stepTimer.start();
}

void TimerField::stepTimerStop()
{
  stepTimer.stop();
}

void TimerField::setStepFrequency(unsigned f)
{
  stepTimer.setFrequency(f);
}

bool TimerField::stepTimerIsRunning() const
{
  return stepTimer.isRunning();
}

// Acceleration Timer ------------------------------------------------------

void TimerField::accTimerStart()
{
  delayISR(accLoopDelayChannel);
}

void TimerField::setAccUpdatePeriod(unsigned p) 
{
  accUpdatePeriod = p;
}

void TimerField::accTimerStop()
{
  accUpdatePeriod = 0;
}

// Delay Timer ------------------------------------------------------

void TimerField::setPulseWidth(unsigned delay)
{
  delayWidth = TeensyStepFTM::microsToReload(delay);
}

void TimerField::triggerDelay()
{
    TeensyStepFTM::trigger(delayWidth, pinResetDelayChannel);
}

void TimerField::delayISR(unsigned channel)
{
  if (channel == pinResetDelayChannel)
  {
    handler->pulseTimerISR();
  }

  else if (channel == accLoopDelayChannel)
  {
    if (accUpdatePeriod == 0) return;

    noInterrupts();
    TeensyStepFTM::trigger(accUpdatePeriod, accLoopDelayChannel);
    interrupts();

    handler->accTimerISR();
  }
}