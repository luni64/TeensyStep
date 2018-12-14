#pragma once

#include "MotorControlBase.h"
#include "Stepper.h"
#include <algorithm>

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControlBase : public MotorControlBase
{
  public:
    //----------------------------------------------------------------------------
    // Starts a  stopping sequence. The motors will be stopped using the normal
    // decelleration. Synchronizity is kept during stopping.
    void stopAsync();

    //----------------------------------------------------------------------------
    // Pass in up to 3 stepper pointers and start synchronous rotation
    // The relative speed of the motor is defined by the corresponding
    // max speed setting
    void rotateAsync(Stepper &stepper0);
    void rotateAsync(Stepper &stepper0, Stepper &stepper1);
    void rotateAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2);

    void moveAsync(Stepper &stepper0)
    {
        mode = Mode::moving;

        motorList[0] = &stepper0;
        motorList[1] = nullptr;
        doMove(1);
    }

    //-----------------------------------------------------------------------------
    // pass in n array of Stepper pointers without passing the size of the array and
    // start synchronous rotation. The relative speed of the motor is defined by
    // the max speed settings of the motors.
    // Example:
    // Stepper M1, M2, M3;
    // Stepper* steppers[] = {&M1, &M2, &M3};
    // rotateAsync(steppers);
    template <size_t N>
    void rotateAsync(Stepper *(&motors)[N]);

  protected:
    RotateControlBase() = default;
    RotateControlBase(const RotateControlBase &) = delete;
    RotateControlBase &operator=(const RotateControlBase &) = delete;

    int32_t virtual updateSpeed(int32_t currentPosition) = 0;
    uint32_t virtual initiateStopping(int32_t currentPosition) = 0;
    void virtual prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration) = 0;
    void virtual prepareMove(int32_t currentPosition, int32_t targetPosition, int32_t targetSpeed, uint32_t acceleration) = 0;

    void doRotate(int N);
    void doMove(int N, bool mode = true);

    void pitISR();
    void delayISR(unsigned channel);

    int oldSpeed = 0;
    enum class Mode
    {
        moving, rotating
    } mode;
};

// Implementation *************************************************************************************************

// ISR -----------------------------------------------------------------------------------------------------------
template <unsigned p, unsigned u>
void RotateControlBase<p, u>::pitISR()
{
    Stepper **motor = motorList; // move leading axis
    (*motor)->doStep();          // activate step pin

    while (*(++motor) != nullptr) // move slow axes if required (https://en.wikipedia.org/wiki/Bresenham)
    {
        if ((*motor)->D >= 0)
        {
            (*motor)->doStep();
            (*motor)->D -= (*motor)->leadTarget; // do not use leadMotor->target since this will be changed by stop()
        }
        (*motor)->D += (*motor)->target;
    }
    TeensyDelay::trigger(p, pinResetDelayChannel); // start delay line to dactivate all step pins

    if(mode == Mode::moving && leadMotor->target == leadMotor->current)
    {     
        StepTimer.stop();     
    }
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

        cli();
        TeensyDelay::trigger(u, accLoopDelayChannel); // retrigger
        sei();

        digitalWriteFast(14, HIGH);
        int32_t newSpeed = updateSpeed(leadMotor->current); // get new speed for the leading motor

        if (oldSpeed == newSpeed) // nothing changed, just keep running
        {
            digitalWriteFast(14, LOW);
            return;
        }
        if (newSpeed == 0) // stop pulsing
        {
            StepTimer.stop();
            oldSpeed = 0;
            digitalWriteFast(14, LOW);
            return;
        }

        int dir = newSpeed >= 0 ? 1 : -1;
        if (dir != leadMotor->dir) // direction of lead motor changed, toggle direction of all slave motors
        {
            Stepper **motor = motorList;
            while ((*motor) != nullptr)
            {
                (*motor++)->toggleDir();
            }
        }

        StepTimer.setFrequency(newSpeed > 0 ? newSpeed : -newSpeed); // speed changed, update timer

        if (oldSpeed == 0) // timer was off -> restart timer
        {
            StepTimer.start();
        }

        oldSpeed = newSpeed;
        digitalWriteFast(14, LOW);
    }
}

// ROTATE Commands -------------- ------------------------------------------------------------------

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::rotateAsync(Stepper &stepper0)
{
    motorList[0] = &stepper0;
    motorList[1] = nullptr;
    doRotate(1);
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::rotateAsync(Stepper &stepper0, Stepper &stepper1)
{
    motorList[0] = &stepper0;
    motorList[1] = &stepper1;
    motorList[2] = nullptr;
    doRotate(2);
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::rotateAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2)
{
    motorList[0] = &stepper0;
    motorList[1] = &stepper1;
    motorList[2] = &stepper2;
    motorList[3] = nullptr;
    doRotate(3);
}

template <unsigned p, unsigned u>
template <size_t N>
void RotateControlBase<p, u>::rotateAsync(Stepper *(&motors)[N])
{
    static_assert((N + 1) <= sizeof(motorList) / sizeof(motorList[0]), "Too many motors used, please increase MaxMotors");

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
void RotateControlBase<p, u>::doMove(int N, bool move)
{
    //Calculate Bresenham parameters ----------------------------------------------------------------
    std::sort(motorList, motorList + N, Stepper::cmpDelta); // The motor which does most steps leads the movement, move to top of list
    leadMotor = motorList[0];

    //leadMotor->current = 0;
    for (int i = 1; i < N; i++)
    {
      //  motorList[i]->current = 0;
        motorList[i]->leadTarget = leadMotor->target;
        motorList[i]->D = 2 * motorList[i]->target - leadMotor->target;
    }

    // Rotation parameters --------------------------------------------------------------
    uint32_t acceleration = (*std::min_element(motorList, motorList + N, Stepper::cmpAcc))->a; // use the lowest acceleration for the move
                                                                                               // uint32_t pullInSpeed = (*std::min_element(motorList, motorList + N, Stepper::cmpVpullIn))->v_pullIn; // use the lowest pull in frequency for the move
    uint32_t targetSpeed = (*std::max_element(motorList, motorList + N, Stepper::cmpV))->vMax; // use the highest vMax for the move
    if (targetSpeed == 0)
        return;

    //prepareRotation(leadMotor->position, targetSpeed, acceleration);
    prepareMove(leadMotor->position, leadMotor->target, targetSpeed, acceleration); 

    TeensyDelay::trigger(2, accLoopDelayChannel); // start the acceleration update loop
}

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::doRotate(int N)
{
    uint32_t maxSpeed = (*std::max_element(motorList, motorList + N, Stepper::cmpV))->vMax;
    float fac = (float)std::numeric_limits<int32_t>::max() / 2.0 / maxSpeed;

    for (int i = 0; i < N; i++)
    {
        motorList[i]->setTargetRel(motorList[i]->vMax * fac * motorList[i]->direction);
    }

    doMove(N, false);
}
