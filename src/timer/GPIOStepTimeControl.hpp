#pragma once

#include "TimerArray.hpp"

/**
 * Step timing controllers manage the timer setups for step generation.
 */
class GPIOStepTimeControl
{
  public:
    inline GPIOStepTimeControl(TimerArrayControl& _control, Timer& _stepTimer, Timer& _delayTimer, Timer& _accTimer);
    inline ~GPIOStepTimeControl();

    inline bool begin();
    inline void end();

    inline void stepTimerStart() { control.attachTimer(&stepTimer); }
    inline void stepTimerStop() { control.detachTimer(&stepTimer); }
    inline bool stepTimerIsRunning() { return stepTimer.isRunning(); }
    inline void setStepFrequency(unsigned f) { stepFrequency = control.fcnt / f; control.changeTimerDelay(&stepTimer, stepFrequency); }
    inline unsigned getStepFrequency() { return stepFrequency; } // returns the actual step frequency

    inline void accTimerStart() { control.attachTimer(&accTimer); }
    inline void accTimerStop() { control.detachTimer(&accTimer); }
    inline void setAccUpdatePeriod(uint32_t period) { control.changeTimerDelay(&accTimer, period); }

    inline void setPulseWidth(uint32_t delay) { control.changeTimerDelay(&delayTimer, delay); }
    inline void triggerDelay() { control.attachTimer(&delayTimer); }

  protected:
    uint32_t stepFrequency;

    TimerArrayControl& control;
    Timer& stepTimer, delayTimer, accTimer;
};

// IMPLEMENTATION ====================================================================

GPIOStepTimeControl::GPIOStepTimeControl(TimerArrayControl& _control, Timer& _stepTimer, Timer& _delayTimer, Timer& _accTimer)
    : control(_control),
      stepTimer(_stepTimer),
      delayTimer(_delayTimer),
      accTimer(_accTimer)
{}

GPIOStepTimeControl::~GPIOStepTimeControl()
{
  end();
}

bool GPIOStepTimeControl::begin()
{
    control.begin();
    return true;
}

void GPIOStepTimeControl::end()
{
    control.stop();
}
