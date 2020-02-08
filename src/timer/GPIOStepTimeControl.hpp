#pragma once

#include "TimerArray.hpp"

/**
 * Step timing controllers manage the timer setups for step generation.
 */
class GPIOStepTimeControl
{
  public:
    inline GPIOStepTimeControl(TimerArrayControl& _control, Timer* _stepTimer, Timer* _delayTimer, Timer* _accTimer);
    inline ~GPIOStepTimeControl();

    inline bool begin();
    inline void end();

    inline void stepTimerStart() { control.attachTimer(stepTimer); }
    inline void stepTimerStop() { control.detachTimer(stepTimer); }
    inline bool stepTimerIsRunning() { return stepTimer->isRunning(); }
    inline void setStepFrequency(unsigned f) {
      if (f == 0){
        stepTimerStop();
        return;
      }
      
      stepPeriod = ((float)control.fclk / control.clkdiv) / f;
      control.changeTimerDelay(stepTimer, stepPeriod);

      if (!stepTimerIsRunning()) stepTimerStart();
    }
    inline unsigned getStepFrequency() { return ((float)control.fclk / control.clkdiv) / stepPeriod; } // returns the actual step frequency

    inline void accTimerStart() { control.attachTimer(accTimer); control.manualFire(accTimer); }
    inline void accTimerStop() { control.detachTimer(accTimer); }
    inline bool accTimerIsRunning() { return accTimer->isRunning(); }
    inline void setAccUpdatePeriod(uint32_t period) { control.changeTimerDelay(accTimer, period); }

    inline void setPulseWidth(uint32_t delay) { control.changeTimerDelay(delayTimer, delay); }
    inline void triggerDelay() { control.attachTimerInSync(delayTimer, stepTimer); }

  protected:
    uint32_t stepPeriod;

    TimerArrayControl& control;
    Timer* stepTimer; Timer* delayTimer; Timer* accTimer;
};

// IMPLEMENTATION ====================================================================

GPIOStepTimeControl::GPIOStepTimeControl(TimerArrayControl& _control, Timer* _stepTimer, Timer* _delayTimer, Timer* _accTimer)
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
