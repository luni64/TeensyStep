#pragma once

//#include <algorithm>
#include <cstdint>
#include <cstddef> // include size_t

#include "timer/TimerArray.hpp"

template <typename Accelerator, typename MotorControl>
class PositionControlBase : public MotorControl
{
public:
    using Stepper = typename MotorControl::Stepper;

    PositionControlBase(TimerArrayControl& _timerControl, uint32_t pulseWidth = 5, uint32_t accUpdatePeriod = 5000);

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
    static void accTimerISR(PositionControlBase* ctx);

    void doMove(int32_t N, bool mode = true);

    ContextTimer<PositionControlBase> accTimer;
    Accelerator accelerator;

    PositionControlBase(const PositionControlBase &) = delete;
    PositionControlBase &operator=(const PositionControlBase &) = delete;
};

// Implementation *************************************************************************************************

template <typename a, typename MotorControl>
PositionControlBase<a, MotorControl>::PositionControlBase(TimerArrayControl& _timerControl, uint32_t pulseWidth, uint32_t accUpdatePeriod)
    : MotorControl(_timerControl, pulseWidth, accUpdatePeriod, accTimer),
    accTimer(this, accTimerISR)
{
    this->mode = MotorControl::Mode::target;
}

template <typename a, typename t>
void PositionControlBase<a, t>::doMove(int32_t N, bool move)
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
    this->stepTimeControl.setStepFrequency(accelerator.prepareMovement(this->leadMotor->current, this->leadMotor->target, targetSpeed, acceleration));
    this->stepTimeControl.stepTimerStart();
    this->stepTimeControl.accTimerStart();
}

// ISR -----------------------------------------------------------------------------------------------------------

template <typename a, typename t>
void PositionControlBase<a, t>::accTimerISR(PositionControlBase* ctx)
{
    if (ctx->isRunning())
    {
        ctx->stepTimeControl.setStepFrequency(ctx->accelerator.updateSpeed(ctx->leadMotor->current));
    }
}

// MOVE ASYNC Commands -------------------------------------------------------------------------------------------------

template <typename a, typename t>
template <typename... Steppers>
void PositionControlBase<a, t>::moveAsync(Steppers &... steppers)
{
    this->attachStepper(steppers...);
    doMove(sizeof...(steppers));
}

template <typename a, typename t>
template <size_t N>
void PositionControlBase<a, t>::moveAsync(Stepper *(&motors)[N]) //move up to maxMotors motors synchronously
{
    this->attachStepper(motors);
    doMove(N);
}

// MOVE Commands -------------------------------------------------------------------------------------------------

template <typename a, typename t>
template <typename... Steppers>
void PositionControlBase<a, t>::move(Steppers &... steppers)
{
    moveAsync(steppers...);
    while (this->stepTimeControl.stepTimerIsRunning())
    {
        HAL_Delay(1);
    }
}

template <typename a, typename t>
template <size_t N>
void PositionControlBase<a, t>::move(Stepper *(&motors)[N])
{
    moveAsync(motors);
    while (this->isRunning())
    {
        HAL_Delay(1);
    }
}

template <typename a, typename t>
void PositionControlBase<a, t>::stopAsync()
{
    uint32_t newTarget = accelerator.initiateStopping(this->leadMotor->current);
    this->leadMotor->target = this->leadMotor->current + this->leadMotor->dir * newTarget;
}

template <typename a, typename t>
void PositionControlBase<a, t>::stop()
{
    stopAsync();
    while (this->isRunning())
    {
        HAL_Delay(1);
    }
}
