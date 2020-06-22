#pragma once

#include "../TF_Handler.h"
#include "PIT.h"
#include "TeensyStepFTM.h"

//===================================================================
// Teensy 3.X
//===================================================================

class TimerField : public IDelayHandler
{
 public:
    inline TimerField(TeensyStep::TF_Handler*);
    inline ~TimerField();

    inline bool begin();
    inline void end();
    inline void endAfterPulse(); // not nice look for better solution

    inline void stepTimerStart();
    inline void stepTimerStop();
    inline bool stepTimerIsRunning() const;
    inline bool stepTimerIsAllocated() const;
    inline void setStepFrequency(unsigned f);
    inline unsigned getStepFrequency();

    inline void accTimerStart();
    inline void accTimerStop();
    inline void setAccUpdatePeriod(unsigned period);

    inline void setPulseWidth(unsigned delay);
    inline void triggerDelay();
    inline void delayISR(unsigned channel);

 protected:
    TeensyStep::PIT stepTimer;
    TeensyStep::TF_Handler* handler;

    unsigned delayWidth;
    unsigned accUpdatePeriod;

    unsigned accLoopDelayChannel;
    unsigned pinResetDelayChannel;

    bool lastPulse = false;
};

// IMPLEMENTATION ====================================================================

TimerField::TimerField(TeensyStep::TF_Handler* _handler)
    : handler(_handler)
{}

TimerField::~TimerField()
{
    end(); // release timer resources
}

bool TimerField::begin()
{
    //digitalWriteFast(4,HIGH);
    lastPulse = false;
    accLoopDelayChannel = TeensyStepFTM::addDelayChannel(this);
    pinResetDelayChannel = TeensyStepFTM::addDelayChannel(this);
    TeensyStepFTM::begin();
    return stepTimer.begin(handler) == TeensyStep::pitErr::OK;
}

// releases all allocated timers (PIT + FTM)
void TimerField::end()
{
    stepTimer.end();
    TeensyStepFTM::removeDelayChannel(accLoopDelayChannel);
    TeensyStepFTM::removeDelayChannel(pinResetDelayChannel);
}

void TimerField::endAfterPulse()
{
    lastPulse = true;
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

            if (ldval < F_BUS / 250) // normal step freqency (> 250 Hz) -> set new period for following periods
            {
                stepTimer.setNextReload(F_BUS / f);
                return;
            }

            uint32_t newReload = F_BUS / f;
            uint32_t cyclesSinceLastStep = ldval - stepTimer.getCVAL();
            if (cyclesSinceLastStep <= newReload) // time between last pulse and now less than required new period -> wait
            {
                stepTimer.setThisReload(newReload - cyclesSinceLastStep);
                stepTimer.setNextReload(newReload);
            } else
            {
                stepTimer.setThisReload(newReload);
                handler->stepTimerISR();
            }
        } else // not running
        {
            stepTimer.setThisReload(F_BUS / f); // restarts implicitly
        }
    } else //f==0
    {
        stepTimer.stop();
    }
}

bool TimerField::stepTimerIsRunning() const
{
    return stepTimer.isRunning();
}

bool TimerField::stepTimerIsAllocated() const
{
    return stepTimer.isAllocated();
}

// Acceleration Timer ------------------------------------------------------

void TimerField::accTimerStart()
{
    TeensyStepFTM::trigger(accUpdatePeriod, accLoopDelayChannel);
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
        if (lastPulse) end();
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