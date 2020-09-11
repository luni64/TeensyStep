#pragma once

#include <algorithm>
#include <cstdint>
#include <cstddef> // include size_t

#include "timer/TimerArray.hpp"

template <typename Accelerator, typename MotorControl>
class SpeedControlBase : public MotorControl
{
  public:
    using Stepper = typename MotorControl::Stepper;

    SpeedControlBase(TimerArrayControl& _timerControl, uint32_t pulseWidth = 25, uint32_t accUpdatePeriod = 5000);

    // Non-blocking movements ----------------
    template <typename... Steppers>
    void rotateAsync(Steppers &... steppers);
   
    template <size_t N>
    void rotateAsync(Stepper *(&motors)[N]);

    bool isMoving();
    void stopAsync();

    void emergencyStop() {
        accelerator.eStop();
        this->stepTimeControl.stepTimerStop();
    }

    // Blocking movements --------------------
    void stop();

    void overrideSpeed(float speedFac);
    void overrideAcceleration(float accFac);

protected:
    void doRotate(int N, float speedFactor = 1.0);
    static void accTimerISR(SpeedControlBase* ctx);

    ContextTimer<SpeedControlBase> accTimer;
    Accelerator accelerator;
    TimerArrayControl& _timerControl;

    SpeedControlBase(const SpeedControlBase &) = delete;
    SpeedControlBase &operator=(const SpeedControlBase &) = delete;
};

// Implementation *************************************************************************************************

template <typename a, typename t>
bool SpeedControlBase<a, t>::isMoving()
{ 
  return accelerator.isMoving();
}

template <typename a, typename MotorControl>
SpeedControlBase<a, MotorControl>::SpeedControlBase(TimerArrayControl& _timerControl, uint32_t pulseWidth, uint32_t accUpdatePeriod)
    : MotorControl(_timerControl, pulseWidth, accUpdatePeriod, &accTimer),
    accTimer(0, true, this, accTimerISR),
    _timerControl(_timerControl)
{
    this->mode = MotorControl::Mode::notarget;
}

template <typename a, typename t>
void SpeedControlBase<a, t>::doRotate(int N, float speedFactor)
{
     //Calculate Bresenham parameters ----------------------------------------------------------------
    std::sort(this->motorList, this->motorList + N, Stepper::cmpVmax);
    this->leadMotor = this->motorList[0];

    if (this->leadMotor->vMax == 0)
        return;

    this->leadMotor->currentSpeed = 0; 

    this->leadMotor->A = std::abs(this->leadMotor->vMax);
    for (int i = 1; i < N; i++)
    {
        this->motorList[i]->A = std::abs(this->motorList[i]->vMax);
        this->motorList[i]->B = 2 * this->motorList[i]->A - this->leadMotor->A;
    }
    uint32_t acceleration = (*std::min_element(this->motorList, this->motorList + N, Stepper::cmpAcc))->a; // use the lowest acceleration for the move
    
    // Start moving---------------------------------------------------------------------------------------  
    accelerator.prepareRotation(this->leadMotor->current, this->leadMotor->vMax, acceleration, this->accUpdatePeriod, speedFactor);
    this->stepTimeControl.setStepFrequency(0);
    this->stepTimeControl.accTimerStart();
}

// ISR -----------------------------------------------------------------------------------------------------------

template <typename a, typename t>
void SpeedControlBase<a, t>::accTimerISR(SpeedControlBase* ctx)
{   
    int32_t newSpeed = ctx->accelerator.updateSpeed(ctx->leadMotor->current); // get new speed for the leading motor
     
    //Serial.printf("rc,curSpeed: %i newspd:%i\n",this->leadMotor->currentSpeed,  newSpeed);

    if (ctx->leadMotor->currentSpeed == newSpeed)
    {         
        return; // nothing changed, just keep running
    }

    int dir = newSpeed >= 0 ? 1 : -1; // direction changed? -> toggle direction of all motors
    if (dir != ctx->leadMotor->dir)
    {
        Stepper **motor = ctx->motorList;
        while ((*motor) != nullptr)
        {
            (*motor++)->toggleDir();
        }
        //delayMicroseconds(this->pulseWidth); // why is this necessary?
    }
    
    
    ctx->stepTimeControl.setStepFrequency(std::abs(newSpeed)); // speed changed, update timer    
    ctx->leadMotor->currentSpeed = newSpeed;   
}

// ROTATE Commands -------------------------------------------------------------------------------

template <typename a, typename t>
template <typename... Steppers>
void SpeedControlBase<a, t>::rotateAsync(Steppers &... steppers)
{
    _timerControl.disableInterrupt();
    this->attachStepper(steppers...);
    doRotate(sizeof...(steppers));
    _timerControl.enableInterrupt();
}

template <typename a, typename t>
template <size_t N>
void SpeedControlBase<a, t>::rotateAsync(Stepper *(&steppers)[N]) 
{
    _timerControl.disableInterrupt();
    this->attachStepper(steppers);
    doRotate(N);
    _timerControl.enableInterrupt();
}

template <typename a, typename t>
void SpeedControlBase<a, t>::overrideSpeed(float factor)
{
    _timerControl.disableInterrupt();
    accelerator.overrideSpeed(factor);
    _timerControl.enableInterrupt();
}

template <typename a, typename t>
void SpeedControlBase<a, t>::overrideAcceleration(float factor)
{
    _timerControl.disableInterrupt();
    accelerator.overrideAcceleration(factor);
    _timerControl.enableInterrupt();
}

template <typename a, typename t>
void SpeedControlBase<a, t>::stopAsync()
{
    _timerControl.disableInterrupt();
    accelerator.initiateStopping(this->leadMotor->current);
    _timerControl.enableInterrupt();
}

template <typename a, typename t>
void SpeedControlBase<a, t>::stop()
{   
    stopAsync();
    while (SpeedControlBase<a, t>::isMoving())
    {
        HAL_Delay(1); // not strictly necessary
    }
}