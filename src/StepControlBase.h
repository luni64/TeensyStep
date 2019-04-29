#pragma once

#include "MotorControlBase.h"
#include <algorithm>

template <typename Accelerator, typename TimerField>
class StepControlBase : public MotorControlBase<TimerField>
{
public:
    StepControlBase(unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000);

    // Non-blocking movements ----------------
    template <typename... Steppers>
    void moveAsync(Steppers &... steppers);

    template <size_t N>
    void moveAsync(Stepper *(&motors)[N]);
    void stopAsync();

    // Blocking movements --------------------
    template <typename... Steppers>
    void move(Steppers &... steppers);

    template <size_t N>
    void move(Stepper *(&motors)[N]);
    void stop();

    // Misc ----------------------------------
    void setCallback(void (*_callback)()) { this->callback = _callback; }

protected:
    void accTimerISR();

    void doMove(int N, bool mode = true);

    Accelerator accelerator;

    StepControlBase(const StepControlBase &) = delete;
    StepControlBase &operator=(const StepControlBase &) = delete;
};

// Implementation *************************************************************************************************

template <typename a, typename t>
StepControlBase<a, t>::StepControlBase(unsigned pulseWidth, unsigned accUpdatePeriod)
    : MotorControlBase<t>(pulseWidth, accUpdatePeriod)
{
    this->mode = MotorControlBase<t>::Mode::target;
}

template <typename a, typename t>
void StepControlBase<a, t>::doMove(int N, bool move)
{
    //Calculate Bresenham parameters ----------------------------------------------------------------
    std::sort(this->motorList, this->motorList + N, Stepper::cmpDelta); // The motor which does most steps leads the movement, move to top of list
    this->leadMotor = this->motorList[0];

    for (int i = 1; i < N; i++)
    {
        this->motorList[i]->B = 2 * this->motorList[i]->A - this->leadMotor->A;
    }

    // Calculate acceleration parameters --------------------------------------------------------------
    uint32_t targetSpeed = std::abs((*std::min_element(this->motorList, this->motorList + N, Stepper::cmpVmin))->vMax); // use the lowest max frequency for the move, scale by relSpeed
    uint32_t acceleration = (*std::min_element(this->motorList, this->motorList + N, Stepper::cmpAcc))->a;              // use the lowest acceleration for the move
    if (this->leadMotor->A == 0 || targetSpeed == 0)
        return;

    // Start move---------------------------------------------------------------------------------------
    this->timerField.setStepFrequency(accelerator.prepareMovement(this->leadMotor->current, this->leadMotor->target, targetSpeed, acceleration));
    this->timerField.stepTimerStart();
    this->timerField.accTimerStart();
}

// ISR -----------------------------------------------------------------------------------------------------------

template <typename a, typename t>
void StepControlBase<a, t>::accTimerISR()
{
    if (this->isRunning())
    {
        this->timerField.setStepFrequency(accelerator.updateSpeed(this->leadMotor->current));
    }
}

// MOVE ASYNC Commands -------------------------------------------------------------------------------------------------

template <typename a, typename t>
template <typename... Steppers>
void StepControlBase<a, t>::moveAsync(Steppers &... steppers)
{
    this->attachStepper(steppers...);
    doMove(sizeof...(steppers));
}

template <typename a, typename t>
template <size_t N>
void StepControlBase<a, t>::moveAsync(Stepper *(&motors)[N]) //move up to maxMotors motors synchronously
{
    this->attachStepper(motors);
    doMove(N);
}

// MOVE Commands -------------------------------------------------------------------------------------------------

template <typename a, typename t>
template <typename... Steppers>
void StepControlBase<a, t>::move(Steppers &... steppers)
{
    moveAsync(steppers...);
    while (this->timerField.stepTimerIsRunning())
    {
        delay(1);
    }
}

template <typename a, typename t>
template <size_t N>
void StepControlBase<a, t>::move(Stepper *(&motors)[N])
{
    moveAsync(motors);
    while (this->isRunning())
    {
        delay(1);
    }
}

template <typename a, typename t>
void StepControlBase<a, t>::stopAsync()
{
    uint32_t newTarget = accelerator.initiateStopping(this->leadMotor->current);
    this->leadMotor->target = this->leadMotor->current + this->leadMotor->dir * newTarget;
}

template <typename a, typename t>
void StepControlBase<a, t>::stop()
{
    stopAsync();
    while (this->isRunning())
    {
        delay(1);
    }
}
