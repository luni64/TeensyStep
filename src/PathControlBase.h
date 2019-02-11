#pragma once

#include "MotorControlBase.h"
#include "PathStepper.h"
#include <cmath>

template <typename TimerField>
class PathControlBase : public MotorControlBase<TimerField>
{
  public:
    PathControlBase(unsigned dt = 10, unsigned pulseLength = 3, unsigned accPeriod=5000) : MotorControlBase<TimerField>(pulseLength, accPeriod)
    {
        this->dt = dt;
        frequency = 1'000'000 / dt;
        this->timerField.setStepFrequency(frequency);
    }

    template <typename... Steppers>
    void followPathAsync(Steppers &... steppers);
    void stop() { this->timerField.stepTimerStop(); }

    void accTimerISR(){}
    void stepTimerISR();
    
    unsigned frequency;
    unsigned dt;
    uint64_t tCnt;
};

template <typename tf>
template <typename... Steppers>
void PathControlBase<tf>::followPathAsync(Steppers &... steppers)
{
    this->attachStepper(steppers...);
    this->timerField.stepTimerStart();        
}

template <typename TimerField>
void PathControlBase<TimerField>::stepTimerISR()
{
    digitalWriteFast(4, HIGH); // to check the calulation time on a scope

    float t = (tCnt * dt)* 1E-6; // Time in seconds

    PathStepper **axis = (PathStepper **) this->motorList;

    while (*(axis) != nullptr)        
    {    
        float target = (*axis)->scaleFactor * (*axis)->pathFunction(t);
        float delta =   target - (*axis)->getPosition();

        if (delta > 1.0f)
        {
            (*axis)->setDir(1);
            (*axis)->doStep();
        }
        else if (delta < -1.0f)
        {
            (*axis)->setDir(-1);
            (*axis)->doStep();
        }
        axis++;
    }
    this->timerField.triggerDelay(); // start delay line to dactivate all step pins
    
    tCnt++;
    digitalWriteFast(4, LOW);
}
