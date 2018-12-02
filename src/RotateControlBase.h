#pragma once

#include "./PIT/PIT.h"
#include "./TeensyDelay/TeensyDelay.h"
#include "Stepper.h"
#include <algorithm>

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControlBase : public MotorControlBase<pulseWidth, accUpdatePeriod>
{
  public:
    void stopAsync();
    void rotateAsync(Stepper &stepper0);
    void rotateAsync(Stepper &stepper0, Stepper &stepper1);
    void rotateAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2);
   
    template <size_t N>
    void rotateAsync(Stepper *(&motors)[N]);

    void setCallback(void (*_callback)()) { callback = _callback; }

  protected:
    RotateControlBase();
    uint32_t virtual updateSpeed(uint32_t currentPosition) = 0;
    uint32_t virtual initiateStopping(uint32_t currentPosition) = 0;
    uint32_t virtual prepareRotation(uint32_t currentPosition, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t acceleration) = 0;

    void doRotate(int N);
    void doMove(int N, bool mode = true);

    void pitISR();
    void delayISR(unsigned channel);

    void (*callback)() = nullptr;
};

// Implementation *************************************************************************************************

template <unsigned p, unsigned u>
RotateControlBase<p, u>::RotateControlBase() : MotorControlBase<p, u>()
{
}

// ISR -----------------------------------------------------------------------------------------------------------
template <unsigned p, unsigned u>
void RotateControlBase<p, u>::pitISR()
{
    Stepper **motor = this->motorList; // move leading axis
    (*motor)->doStep();                // activate step pin

    while (*(++motor) != nullptr) // move slow axes if required (https://en.wikipedia.org/wiki/Bresenham)
    {
        if ((*motor)->D >= 0)
        {
            (*motor)->doStep();
            (*motor)->D -= (*motor)->leadTarget; // do not use motorlist[0]->target since this will be changed by stop()
        }
        (*motor)->D += (*motor)->target;
    }
    TeensyDelay::trigger(p, this->pinResetDelayChannel); // start delay line to dactivate all step pins

    // if (this->motorList[0]->current >= this->motorList[0]->target) // stop if we are at target
    // {
    //     this->StepTimer.channel->TCTRL &= ~PIT_TCTRL_TIE; // disable timer interrupts
    //     if (callback != nullptr)
    //         callback();
    // }
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::delayISR(unsigned channel)
{
    //clear all step pins ----------------------------------------------
    if (channel == this->pinResetDelayChannel)
    {
        Stepper **motor = this->motorList;
        while ((*motor) != nullptr)
        {
            (*motor++)->clearStepPin();
        }
    }

    // calculate new speed (i.e., timer reload value) -------------------
    if (channel == this->accLoopDelayChannel)
    {
        if (this->StepTimer.isRunning())
        {
            cli();
            TeensyDelay::trigger(u, this->accLoopDelayChannel); // retrigger
            sei();

            uint32_t newSpeed = updateSpeed(this->motorList[0]->current);
            this->StepTimer.channel->LDVAL = F_BUS / newSpeed;
        }
    }
}

// ROTATE Commands -----------------------------------------------------------------------------------------------

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::rotateAsync(Stepper &stepper0)
{
    this->motorList[0] = &stepper0;
    this->motorList[1] = nullptr;
    doRotate(1);
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::rotateAsync(Stepper &stepper0, Stepper &stepper1)
{
    this->motorList[0] = &stepper0;
    this->motorList[1] = &stepper1;
    this->motorList[2] = nullptr;
    doRotate(2);
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::rotateAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2)
{
    this->motorList[0] = &stepper0;
    this->motorList[1] = &stepper1;
    this->motorList[2] = &stepper2;
    this->motorList[3] = nullptr;
    doRotate(3);
}

template <unsigned p, unsigned u>
template <size_t N>
inline void RotateControlBase<p, u>::rotateAsync(Stepper *(&motors)[N])
{
    static_assert((N + 1) <= sizeof(this->motorList) / sizeof(this->motorList[0]), "Too many motors used, please increase MaxMotors");

    for (unsigned i = 0; i < N; i++)
    {
        this->motorList[i] = motors[i];
    }
    this->motorList[N] = nullptr;
    doRotate(N);
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::stopAsync()
{
    uint32_t newTarget = initiateStopping(this->motorList[0]->position);
    this->motorList[0]->target = newTarget;
}

// Protected -----------------------------------------------------------------------------------------------------

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::doMove(int N, bool move)
{
    //Calculate Bresenham parameters ----------------------------------------------------------------
    std::sort(this->motorList, this->motorList + N, Stepper::cmpDelta); // The motor which does most steps leads the movement, move to top of list
    this->motorList[0]->current = 0;
    for (int i = 1; i < N; i++)
    {
        this->motorList[i]->current = 0;
        this->motorList[i]->leadTarget = this->motorList[0]->target;
        this->motorList[i]->D = 2 * this->motorList[i]->target - this->motorList[0]->target;
    }

    // Rotation parameters --------------------------------------------------------------
    uint32_t acceleration = (*std::min_element(this->motorList, this->motorList + N, Stepper::cmpAcc))->a;           // use the lowest acceleration for the move
    uint32_t pullInSpeed = (*std::min_element(this->motorList, this->motorList + N, Stepper::cmpVpullIn))->v_pullIn; // use the lowest pull in frequency for the move
    uint32_t targetSpeed = (*std::max_element(this->motorList, this->motorList + N, Stepper::cmpV))->vMax;           // use the highest vMax for the move
    if (targetSpeed == 0)
        return;
    
    uint32_t vstart = prepareRotation(this->motorList[0]->position, targetSpeed, pullInSpeed, acceleration);
    this->StepTimer.channel->LDVAL = F_BUS / vstart;

    // Start timers
    this->StepTimer.clearTIF();
    this->StepTimer.start();
    TeensyDelay::trigger(2, this->accLoopDelayChannel); // start the acceleration update loop
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::doRotate(int N)
{
    uint32_t maxSpeed = (*std::max_element(this->motorList, this->motorList + N, Stepper::cmpV))->vMax;
    float fac = (float)std::numeric_limits<int32_t>::max() / 2.0 / maxSpeed;

    for (int i = 0; i < N; i++)
    {
        this->motorList[i]->setTargetRel(this->motorList[i]->vMax * fac * this->motorList[i]->direction);
    }

    doMove(N, false);
}
