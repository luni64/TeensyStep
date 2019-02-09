#pragma once

#include "MotorControlBase.h"
#include "PathStepper.h"
#include <cmath>

 //constexpr unsigned dt = 10;     //µs
// constexpr float twoPi = TWO_PI; // don't use the double version
// constexpr float rpm = 6;
// constexpr float rad_ps = twoPi * rpm / 60;
// constexpr float rad_pus = rad_ps * 1.0E-6;

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
    void rotateAsync(Steppers &... steppers);
    void stop() { this->timerField.stepTimerStop(); }

    void accTimerISR(){}
    void stepTimerISR();

    
    unsigned frequency;
    unsigned dt;
    uint64_t tCnt;
   
    //unsigned t = 0;
};

template <typename tf>
template <typename... Steppers>
void PathControlBase<tf>::rotateAsync(Steppers &... steppers)
{
    this->attachStepper(steppers...);
    this->timerField.stepTimerStart();    
}

template <typename tf>
void PathControlBase<tf>::stepTimerISR()
{
    digitalWriteFast(4, HIGH); // to check the calulation time on a scope

    float t = (tCnt++) * dt;
    
    PathStepper **axis = (PathStepper **) this->motorList;

    while (*(++axis) != nullptr)
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
    }
    this->timerField.triggerDelay(); // start delay line to dactivate all step pins

    t += dt; 

    digitalWriteFast(4, LOW);
}
