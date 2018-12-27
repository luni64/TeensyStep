#pragma once

#include "MotorControlBase.h"
#include "Stepper.h"
#include <algorithm>

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControlBase : public MotorControlBase
{
  public:
    void rotateAsync(Stepper &stepper);

    template <typename... Steppers>
    void rotateAsync(Stepper &stepper, Steppers &... steppers);

    template <size_t N>
    void rotateAsync(Stepper *(&motors)[N]);

    void stopAsync();

    // //----------------------------------------------------------------------------
    // // Pass in up to 3 stepper pointers and start synchronous rotation
    // // The relative speed of the motor is defined by the corresponding
    // // max speed setting
    // void rotateAsync(Stepper &stepper0);
    // void rotateAsync(Stepper &stepper0, Stepper &stepper1);
    // void rotateAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2);

    // //-----------------------------------------------------------------------------
    // // pass in n array of Stepper pointers without passing the size of the array and
    // // start synchronous rotation. The relative speed of the motor is defined by
    // // the max speed settings of the motors.
    // // Example:
    // // Stepper M1, M2, M3;
    // // Stepper* steppers[] = {&M1, &M2, &M3};
    // // rotateAsync(steppers);
    // template <size_t N>
    // void rotateAsync(Stepper *(&motors)[N]);

  protected:
    RotateControlBase() = default;
    RotateControlBase(const RotateControlBase &) = delete;
    RotateControlBase &operator=(const RotateControlBase &) = delete;

    void doRotate(int N);
    void doMove(int N, bool mode = true);

    void pitISR();
    void delayISR(unsigned channel);

    int32_t virtual updateSpeed(int32_t currentPosition) = 0;
    uint32_t virtual initiateStopping(int32_t currentPosition) = 0;
    int32_t virtual prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration) = 0;    
};

// Implementation *************************************************************************************************

// ISR -----------------------------------------------------------------------------------------------------------
template <unsigned p, unsigned u>
void RotateControlBase<p, u>::pitISR()
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

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::delayISR(unsigned channel)
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

        int32_t newSpeed = updateSpeed(leadMotor->current); // get new speed for the leading motor

        if (leadMotor->currentSpeed == newSpeed) // nothing changed, just keep running
        {
            return;
        }
        if (newSpeed == 0) // stop pulsing
        {
            StepTimer.stop();
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

        StepTimer.setFrequency(std::abs(newSpeed)); // speed changed, update timer

        if (leadMotor->currentSpeed == 0) // timer was off -> restart
        {
            StepTimer.start();
        }
        leadMotor->currentSpeed = newSpeed;
    }
}

// ROTATE Commands -------------- ------------------------------------------------------------------

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::rotateAsync(Stepper &stepper)
{
    motorList[mCnt++] = &stepper;
    motorList[mCnt] = nullptr;
    doRotate(mCnt);
    mCnt = 0;
}

template <unsigned p, unsigned u>
template <typename... Steppers>
void RotateControlBase<p, u>::rotateAsync(Stepper &stepper, Steppers &... steppers)
{
    static_assert(sizeof...(steppers) < MaxMotors, "Too many motors used. Please increase MaxMotors in file MotorControlBase.h");

    motorList[mCnt++] = &stepper;
    rotateAsync(steppers...);
}

template <unsigned p, unsigned u>
template <size_t N>
void RotateControlBase<p, u>::rotateAsync(Stepper *(&motors)[N]) //move up to maxMotors motors synchronously
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

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::stopAsync()
{
    initiateStopping(leadMotor->current);
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::doRotate(int N)
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
    int32_t s = prepareRotation(leadMotor->current, leadMotor->vMax, acceleration);

    StepTimer.setFrequency(s);
    pitISR();  
    delayISR(accLoopDelayChannel);
}
