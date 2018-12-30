#pragma once

#include "MotorControlBase.h"
#include "Stepper.h"

#include <algorithm>
#include "timer/TF_Handler.h"

template <typename Accelerator, typename TimerField, unsigned pulseWidth, unsigned accUpdatePeriod>
class StepControlBase : public MotorControlBase, public TF_Handler
{
  public:
    StepControlBase();
    ~StepControlBase() { timerField.end(); }

    // Blocking movements --------------------
    template <typename... Steppers>
    void move(Steppers &... steppers);

    template <size_t N>
    void move(Stepper *(&motors)[N]);

    // Non-blocking movements ----------------
    void moveAsync(Stepper &stepper);

    template <typename... Steppers>
    void moveAsync(Stepper &stepper, Steppers &... steppers);

    template <size_t N>
    void moveAsync(Stepper *(&motors)[N]);

    // Misc
    void stop();
    void stopAsync();
    void setCallback(void (*_callback)()) { callback = _callback; }

  protected:
    void stepTimerISR();
    void accTimerISR();
    void pulseTimerISR();

    void doMove(int N, bool mode = true);

    void (*callback)() = nullptr;
    Accelerator accelerator;
    TimerField timerField;

    StepControlBase(const StepControlBase &) = delete;
    StepControlBase &operator=(const StepControlBase &) = delete;
};

// Implementation *************************************************************************************************

template <typename a, typename t, unsigned p, unsigned u>
StepControlBase<a, t, p, u>::StepControlBase() : timerField(this)
{
    OK = timerField.begin();
    timerField.setPulseWidth(p);
    timerField.setAccUpdatePeriod(u);
}

template <typename a, typename t, unsigned p, unsigned u>
void StepControlBase<a, t, p, u>::doMove(int N, bool move)
{
    //Calculate Bresenham parameters ----------------------------------------------------------------
    std::sort(motorList, motorList + N, Stepper::cmpDelta); // The motor which does most steps leads the movement, move to top of list
    leadMotor = motorList[0];

    for (int i = 1; i < N; i++)
    {
        motorList[i]->B = 2 * motorList[i]->A - leadMotor->A;
    }

    // Calculate acceleration parameters --------------------------------------------------------------
    uint32_t targetSpeed = std::abs((*std::min_element(motorList, motorList + N, Stepper::cmpVmin))->vMax); // use the lowest max frequency for the move, scale by relSpeed
    uint32_t acceleration = (*std::min_element(motorList, motorList + N, Stepper::cmpAcc))->a;              // use the lowest acceleration for the move
    if (leadMotor->A == 0 || targetSpeed == 0)
        return;

    // Start move---------------------------------------------------------------------------------------
    timerField.setStepFrequency(accelerator.prepareMovement(leadMotor->current, leadMotor->target, targetSpeed, acceleration));
    timerField.stepTimerStart();

    stepTimerISR(); // initiate first step immediately (no need to wait for the potentially long first cycle)
    timerField.accTimerStart();
}

// ISR -----------------------------------------------------------------------------------------------------------
template <typename a, typename t, unsigned p, unsigned u>
void StepControlBase<a, t, p, u>::stepTimerISR()
{
    leadMotor->doStep(); // move master motor

    Stepper **slave = motorList;
    while (*(++slave) != nullptr) // move slave motors if required (https://en.wikipedia.org/wiki/Bresenham)
    {
        if ((*slave)->B >= 0)
        {
            (*slave)->doStep();
            (*slave)->B -= leadMotor->A;
        }
        (*slave)->B += (*slave)->A;
    }
    timerField.triggerDelay(); // start delay line to dactivate all step pins

    if (leadMotor->current == leadMotor->target) // stop timer and call callback if we reached target
    {
        timerField.stepTimerStop();
        if (callback != nullptr)
            callback();
    }
}

template <typename a, typename t, unsigned p, unsigned u>
void StepControlBase<a, t, p, u>::pulseTimerISR()
{
    //clear all step pins ----------------------------------------------

    Stepper **motor = motorList;
    while ((*motor) != nullptr)
    {
        (*motor++)->clearStepPin();
    }
}

template <typename a, typename t, unsigned p, unsigned u>
void StepControlBase<a, t, p, u>::accTimerISR()
{
    if (timerField.stepTimerIsRunning())
    {
        timerField.setStepFrequency(accelerator.updateSpeed(leadMotor->current));
    }
}

// MOVE ASYNC Commands -------------------------------------------------------------------------------------------------

template <typename a, typename t, unsigned p, unsigned u>
void StepControlBase<a, t, p, u>::moveAsync(Stepper &stepper)
{
    motorList[mCnt++] = &stepper;
    motorList[mCnt] = nullptr;
    doMove(mCnt);
    mCnt = 0;
}

template <typename a, typename t, unsigned p, unsigned u>
template <typename... Steppers>
void StepControlBase<a, t, p, u>::moveAsync(Stepper &stepper, Steppers &... steppers)
{
    static_assert(sizeof...(steppers) < MaxMotors, "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

    motorList[mCnt++] = &stepper;
    moveAsync(steppers...);
}

template <typename a, typename t, unsigned p, unsigned u>
template <size_t N>
void StepControlBase<a, t, p, u>::moveAsync(Stepper *(&motors)[N]) //move up to maxMotors motors synchronously
{
    static_assert((N + 1) <= sizeof(motorList) / sizeof(motorList[0]), "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

    unsigned i;
    for (i = 0; i < N; i++)
    {
        if (motors[i] == nullptr)
            break;
        motorList[i] = motors[i];
    }
    motorList[i] = nullptr;
    doMove(i);
}

// MOVE Commands -------------------------------------------------------------------------------------------------

template <typename a, typename t, unsigned p, unsigned u>
template <typename... Steppers>
void StepControlBase<a, t, p, u>::move(Steppers &... steppers)
{
    moveAsync(steppers...);
    while (timerField.stepTimerIsRunning())
    {
        delay(1);
    }
}

template <typename a, typename t, unsigned p, unsigned u>
template <size_t N>
void StepControlBase<a, t, p, u>::move(Stepper *(&motors)[N])
{
    moveAsync(motors);
    while (timerField.stepTimerIsRunning())
    {
        delay(1);
    }
}

template <typename a, typename t, unsigned p, unsigned u>
void StepControlBase<a, t, p, u>::stopAsync()
{
    uint32_t newTarget = accelerator.initiateStopping(leadMotor->current);
    leadMotor->target = newTarget;
}

template <typename a, typename t, unsigned p, unsigned u>
void StepControlBase<a, t, p, u>::stop()
{
    stopAsync();
    while (timerField.stepTimerIsRunning())
    {
        delay(1);
    }
}
