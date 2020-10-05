#if defined(__IMXRT1052__)
#pragma once

#include "wiring.h"
#include "imxrt.h"

 #include "ticktimer.h"
// #include "TeensyStepFTM.h"

#include "../TF_Handler.h"
//#include "../TimerFieldBase.h"

#include "IntervalTimer.h"

//=========================
// Teensy 4
//=========================

class TimerField 
{
public:
  inline TimerField(TF_Handler *);

  inline bool begin();
  inline void end();

  inline void stepTimerStart();
  inline void stepTimerStop();
  inline bool stepTimerIsRunning() const;
  inline void setStepFrequency(unsigned f);
   inline unsigned getStepFrequency() {
     return 0;

   }


   inline void accTimerStart();
   inline void accTimerStop();
   inline void setAccUpdatePeriod(unsigned period);

   inline void setPulseWidth(unsigned delay);
   inline void triggerDelay();
   inline void delayISR(unsigned channel);

   static inline void tick() { tickTimer::tick(); }

 protected:
   //PIT stepTimer;

   tickTimer timer;

   TF_Handler *handler;

   unsigned delayWidth;
   unsigned accUpdatePeriod;

   unsigned accLoopDelayChannel;
   unsigned pinResetDelayChannel;

   unsigned stepFrequency;
};

// IMPLEMENTATION ====================================================================

TimerField::TimerField(TF_Handler *_handler)
    : handler(_handler)
      // accLoopDelayChannel(TeensyStepFTM::addDelayChannel(this)),
      // pinResetDelayChannel(TeensyStepFTM::addDelayChannel(this))
{
//  Serial.println("tf");
accLoopDelayChannel = timer.attachPeriodicTimer(handler->accTimerISR);
}

bool TimerField::begin()
{
  // TeensyStepFTM::begin();
  // return stepTimer.begin(handler);
  return false;
}

void TimerField::end()
{
  // stepTimer.end();
  // TeensyStepFTM::removeDelayChannel(accLoopDelayChannel);
  // TeensyStepFTM::removeDelayChannel(pinResetDelayChannel);
}

// Step Timer ------------------------------------------------------

void TimerField::stepTimerStart()
{
 // stepTimer.start();
}

void TimerField::stepTimerStop()
{
  //stepTimer.stop();
}

void TimerField::setStepFrequency(unsigned f)
{
  // stepFrequency = f;
  // stepTimer.setFrequency(stepFrequency);
}

bool TimerField::stepTimerIsRunning() const
{
  //return stepTimer.isRunning();
  return false;
}


// Acceleration Timer ------------------------------------------------------

void TimerField::accTimerStart()
{
  //delayISR(accLoopDelayChannel);
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
 // delayWidth = TeensyStepFTM::microsToReload(delay);
}

void TimerField::triggerDelay()
{
   // TeensyStepFTM::trigger(delayWidth, pinResetDelayChannel);
}

void TimerField::delayISR(unsigned channel)
{
  // if (channel == pinResetDelayChannel)
  // {
  //   handler->pulseTimerISR();
  // }

  // else if (channel == accLoopDelayChannel)
  // {
  //   if (accUpdatePeriod == 0) return;

  //   noInterrupts();
  // //  TeensyStepFTM::trigger(accUpdatePeriod, accLoopDelayChannel);
  //   interrupts();

  //   handler->accTimerISR();
//  }
}

#endif
