#pragma once

#include "MotorControlBase.h"
#include "Stepper.h"
#include <algorithm>

#include "timer/teensy3/TimerField.h"

template <typename Accelerator, typename Timer, unsigned pulseWidth, unsigned accUpdatePeriod>
class RotateControlBase : public MotorControlBase, public TF_Handler
{
  public:
    RotateControlBase() { OK = timerField.begin(this); }
    ~RotateControlBase() { timerField.end(); }

    void rotateAsync(Stepper &stepper);

    template <typename... Steppers>
    void rotateAsync(Stepper &stepper, Steppers &... steppers);

    template <size_t N>
    void rotateAsync(Stepper *(&motors)[N]);

    void stopAsync();
    void stop();

    void overrideSpeed(float factor);

    RotateControlBase(const RotateControlBase &) = delete;
    RotateControlBase &operator=(const RotateControlBase &) = delete;

  protected:
    Accelerator accelerator;
    Timer timerField;

    void doRotate(int N, float speedFactor = 1.0);

    void stepTimerISR();
    void accTimerISR() {}
    void delayTimerISR() {}

    void delayISR(unsigned channel);
};

// Implementation *************************************************************************************************

// ISR -----------------------------------------------------------------------------------------------------------

template <typename a, typename t, unsigned p, unsigned u>
void RotateControlBase<a, t, p, u>::stepTimerISR()
{
    Stepper **slave = motorList;
    leadMotor->doStep(); // move master motor

    while (*(++slave) != nullptr) // move slave motors if required (https://en.wikipedia.org/wiki/Bresenham)
    {
        if ((*slave)->B >= 0)
        {
            (*slave)->doStep();
            (*slave)->B -= leadMotor->A;
        }
        (*slave)->B += (*slave)->A;
    }
    TeensyDelay::trigger(p, pinResetDelayChannel); // start delay line to dactivate all step pins
}

template <typename a, typename t, unsigned p, unsigned u>
void RotateControlBase<a, t, p, u>::delayISR(unsigned channel)
{
    //clear all step pins ----------------------------------------------
    if (channel == pinResetDelayChannel)
    {
        Stepper **motor = motorList;
        while ((*motor) != nullptr)
        {
            (*motor++)->clearStepPin();
        }
    }

    // calculate new speed  ----------------------------------------------
    if (channel == accLoopDelayChannel)
    {
        noInterrupts();
        TeensyDelay::trigger(u, accLoopDelayChannel); // retrigger
        interrupts();

        int32_t newSpeed = accelerator.updateSpeed(leadMotor->current); // get new speed for the leading motor

        if (leadMotor->currentSpeed == newSpeed) // nothing changed, just keep running
        {
            return;
        }
        if (newSpeed == 0) // stop pulsing
        {
            // StepTimer.stop();
            timerField.stepTimerStop();
            leadMotor->currentSpeed = 0;
            return;
        }

        int dir = newSpeed >= 0 ? 1 : -1; // direction of lead motor changed? -> toggle direction of all motors
        if (dir != leadMotor->dir)
        {
            Stepper **motor = motorList;
            while ((*motor) != nullptr)
            {
                (*motor++)->toggleDir();
            }
        }

        timerField.stepTimerSetFrequency(std::abs(newSpeed)); // speed changed, update timer

        if (leadMotor->currentSpeed == 0) // timer was off -> restart
        {
            timerField.stepTimerStart(); // speed changed, update timer
        }
        leadMotor->currentSpeed = newSpeed;
    }
}

// ROTATE Commands -------------------------------------------------------------------------------

template <class a, typename t, unsigned p, unsigned u>
void RotateControlBase<a, t, p, u>::rotateAsync(Stepper &stepper)
{
    motorList[mCnt++] = &stepper;
    motorList[mCnt] = nullptr;
    doRotate(mCnt);
    mCnt = 0;
}

template <typename a, typename t, unsigned p, unsigned u>
template <typename... Steppers>
void RotateControlBase<a, t, p, u>::rotateAsync(Stepper &stepper, Steppers &... steppers)
{
    static_assert(sizeof...(steppers) < MaxMotors, "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

    motorList[mCnt++] = &stepper;
    rotateAsync(steppers...);
}

template <typename a, typename t, unsigned p, unsigned u>
template <size_t N>
void RotateControlBase<a, t, p, u>::rotateAsync(Stepper *(&motors)[N]) //move up to maxMotors motors synchronously
{
    static_assert((N + 1) <= sizeof(motorList) / sizeof(motorList[0]), "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

    for (unsigned i = 0; i < N; i++)
    {
        motorList[i] = motors[i];
    }
    motorList[N] = nullptr;
    doRotate(N);
}

// Protected -----------------------------------------------------------------------------------------------------

template <typename a, typename t, unsigned p, unsigned u>
void RotateControlBase<a, t, p, u>::overrideSpeed(float factor)
{
    accelerator.overrideSpeed(factor, leadMotor->current);
}

template <typename a, typename t, unsigned p, unsigned u>
void RotateControlBase<a, t, p, u>::stopAsync()
{
    accelerator.initiateStopping(leadMotor->current);
}

template <typename a, typename t, unsigned p, unsigned u>
void RotateControlBase<a, t, p, u>::stop()
{
    stopAsync();
    while (timerField.stepTimerIsRunning())
    {
        delay(1);
    }
}

template <class a, typename t, unsigned p, unsigned u>
void RotateControlBase<a, t, p, u>::doRotate(int N, float speedFactor)
{
    std::sort(motorList, motorList + N, Stepper::cmpVmax);
    leadMotor = motorList[0];

    if (leadMotor->vMax == 0)
        return;

    leadMotor->A = std::abs(leadMotor->vMax);

    for (int i = 1; i < N; i++)
    {
        motorList[i]->A = std::abs(motorList[i]->vMax);
        motorList[i]->B = 2 * motorList[i]->A - leadMotor->A;
    }

    uint32_t acceleration = (*std::min_element(motorList, motorList + N, Stepper::cmpAcc))->a; // use the lowest acceleration for the move
    int32_t f = accelerator.prepareRotation(leadMotor->current, leadMotor->vMax, acceleration, speedFactor);

    timerField.stepTimerSetFrequency(f);
    stepTimerISR();
    delayISR(accLoopDelayChannel);
}
