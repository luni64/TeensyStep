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
  inline ~TimerField();

  inline bool begin();
  inline void end();

  inline void stepTimerStart();
  inline void stepTimerStop();
  inline bool stepTimerIsRunning() const;
  inline void setStepFrequency(unsigned f);
  inline unsigned getStepFrequency();

  inline void accTimerStart();
  inline void accTimerStop();
  inline void setAccUpdatePeriod(unsigned period);

  inline void setPulseWidth(unsigned delay);
  inline void triggerDelay();
  inline void delayISR(unsigned channel);

  //protected:
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

TimerField::~TimerField()
{  
  end(); 
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

unsigned TimerField::getStepFrequency()
{
  return F_BUS / stepTimer.getLDVAL();
}

void TimerField::setStepFrequency(unsigned f)
{  
  if (f != 0)
  {
    if (stepTimer.isRunning())
    {
      uint32_t ldval = stepTimer.getLDVAL();

      if (ldval < F_BUS / 250 ) // normal step freqency (> 250 Hz) -> set new period for following periods
      {       
        stepTimer.setNextReload(F_BUS / f);
        return;
      }
      
      uint32_t newReload = F_BUS / f;
      uint32_t cyclesSinceLastStep = ldval - stepTimer.channel->CVAL;
      if (cyclesSinceLastStep <= newReload) // time between last pulse and now less than required new period -> wait
      {       
        stepTimer.setThisReload(newReload - cyclesSinceLastStep);
        stepTimer.setNextReload(newReload);
      }
      else
      {       
        stepTimer.setThisReload(newReload);
        handler->stepTimerISR();
      }
    }
    else // not running
    {
      handler->stepTimerISR();
      stepTimer.setThisReload(F_BUS / f);  // restarts implicitly
    }
  }
  else //f==0
  {    
    stepTimer.stop();       
  }
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
  accUpdatePeriod = TeensyStepFTM::microsToReload(p);
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
    if (accUpdatePeriod == 0)
      return;

    noInterrupts();
    TeensyStepFTM::trigger(accUpdatePeriod, accLoopDelayChannel);
    interrupts();

    handler->accTimerISR();
  }
}