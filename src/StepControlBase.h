#pragma once

#include "./PIT/PIT.h"
#include "./TeensyDelay/TeensyDelay.h"
#include "Stepper.h"
#include <algorithm>

constexpr int MaxMotors = 10;

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class StepControlBase : IPitHandler, IDelayHandler
{
  public:
    StepControlBase();

    bool isOk() { return OK; }
    bool isRunning() { return StepTimer.channel->TCTRL & PIT_TCTRL_TIE; }

    void move(Stepper &stepper0, float relSpeed = 1);
    void move(Stepper &stepper0, Stepper &stepper1, float relSpeed = 1);
    void move(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2, float relSpeed = 1);
    template <size_t N>
    void move(Stepper *(&motors)[N], float relSpeed = 1);

    void moveAsync(Stepper &stepper0, float relSpeed = 1);
    void moveAsync(Stepper &stepper0, Stepper &stepper1, float relSpeed = 1);
    void moveAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2, float relSpeed = 1);
    template <size_t N>
    void moveAsync(Stepper *(&motors)[N], float relSpeed = 1);

    void rotateAsync(Stepper &stepper0, float relSpeed = 1);
    void rotateAsync(Stepper &stepper0, Stepper &stepper1, float relSpeed = 1);
    void rotateAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2, float relSpeed = 1);
    template <size_t N>
    void rotateAsync(Stepper *(&motors)[N], float relSpeed = 1);

    void stop();
    void stopAsync();
    void emergencyStop() { StepTimer.disableInterupt(); }

    void setCallback(void (*_callback)()) { callback = _callback; }

  protected:
    uint32_t virtual prepareMovement(uint32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t acceleration) = 0;
    uint32_t virtual updateSpeed(uint32_t currentPosition) = 0;
    uint32_t virtual initiateStopping(uint32_t currentPosition) = 0;

    void doRotate(int N, float relSpeed);
    void doMove(int N, float relSpeed, bool mode = true);

    void pitISR();
    void delayISR(unsigned channel);

    bool OK = false;
    PIT StepTimer;
  
    const unsigned pinResetDelayChannel;
    const unsigned accLoopDelayChannel;
    Stepper *motorList[MaxMotors];

    void (*callback)() = nullptr;
};

// Implementation *************************************************************************************************

template <unsigned p, unsigned u>
StepControlBase<p, u>::StepControlBase() : pinResetDelayChannel(TeensyDelay::addDelayChannel(this)), accLoopDelayChannel(TeensyDelay::addDelayChannel(this))
{   
    OK = StepTimer.begin(this);
    TeensyDelay::begin();
}

// ISR -----------------------------------------------------------------------------------------------------------
template <unsigned p, unsigned u>
void StepControlBase<p, u>::pitISR()
{
    Stepper **motor = motorList; // move leading axis
    (*motor)->doStep();          // activate step pin

    while (*(++motor) != nullptr) // move slow axes if required (https://en.wikipedia.org/wiki/Bresenham)
    {
        if ((*motor)->D >= 0)
        {
            (*motor)->doStep();
            (*motor)->D -= (*motor)->leadTarget; // do not use motorlist[0]->target since this will be changed by stop()
        }
        (*motor)->D += (*motor)->target;
    }
    TeensyDelay::trigger(p, pinResetDelayChannel); // start delay line to dactivate all step pins

    if (motorList[0]->current >= motorList[0]->target) // stop if we are at target
    {
        StepTimer.channel->TCTRL &= ~PIT_TCTRL_TIE; // disable timer interrupts
        if (callback != nullptr) callback();         
    }
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::delayISR(unsigned channel)
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

    // calculate new speed (i.e., timer reload value) -------------------
    if (channel == accLoopDelayChannel) 
    {        
        if (StepTimer.isRunning())
        {
            cli();
            TeensyDelay::trigger(u, accLoopDelayChannel); // retrigger
            sei();
         
            uint32_t newSpeed = updateSpeed(motorList[0]->current);
            StepTimer.channel->LDVAL = F_BUS /newSpeed;
        }
    }
}

// MOVE Commands -------------------------------------------------------------------------------------------------

template <unsigned p, unsigned u>
void StepControlBase<p, u>::move(Stepper &stepper, float relSpeed)
{
    moveAsync(stepper, relSpeed);
    while (isRunning())
        delay(10);
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::move(Stepper &stepper1, Stepper &stepper2, float relSpeed)
{
    moveAsync(stepper1, stepper2, relSpeed);
    while (isRunning())
        delay(10);
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::move(Stepper &stepper1, Stepper &stepper2, Stepper &stepper3, float relSpeed)
{
    moveAsync(stepper1, stepper2, stepper3, relSpeed);
    while (isRunning())
        delay(10);
}

template <unsigned p, unsigned u>
template <size_t N>
void StepControlBase<p, u>::move(Stepper *(&motors)[N], float relSpeed)
{
    moveAsync(motors, relSpeed);
    while (isRunning())
        delay(10);
}

// MOVE ASYNC Commands -------------------------------------------------------------------------------------------

template <unsigned p, unsigned u>
void StepControlBase<p, u>::moveAsync(Stepper &stepper, float relSpeed)
{
    motorList[0] = &stepper;
    motorList[1] = nullptr;
    doMove(1, relSpeed);
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::moveAsync(Stepper &stepper0, Stepper &stepper1, float relSpeed)
{
    motorList[0] = &stepper0;
    motorList[1] = &stepper1;
    motorList[2] = nullptr;
    doMove(2, relSpeed);
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::moveAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2, float relSpeed)
{
    motorList[0] = &stepper0;
    motorList[1] = &stepper1;
    motorList[2] = &stepper2;
    motorList[3] = nullptr;
    doMove(3, relSpeed);
}

template <unsigned p, unsigned u>
template <size_t N>
void StepControlBase<p, u>::moveAsync(Stepper *(&motors)[N], float relSpeed) //move up to maxMotors motors synchronously
{
    static_assert((N + 1) <= sizeof(motorList) / sizeof(motorList[0]), "Too many motors used, please increase MaxMotors");

    for (unsigned i = 0; i < N; i++)
    {
        motorList[i] = motors[i];
    }
    motorList[N] = nullptr;
    doMove(N, relSpeed);
}

// ROTATE Commands -----------------------------------------------------------------------------------------------

template <unsigned p, unsigned u>
void StepControlBase<p, u>::rotateAsync(Stepper &stepper0, float relSpeed)
{
    motorList[0] = &stepper0;
    motorList[1] = nullptr;
    doRotate(1, relSpeed);
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::rotateAsync(Stepper &stepper0, Stepper &stepper1, float relSpeed)
{
    motorList[0] = &stepper0;
    motorList[1] = &stepper1;
    motorList[2] = nullptr;
    doRotate(2, relSpeed);
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::rotateAsync(Stepper &stepper0, Stepper &stepper1, Stepper &stepper2, float relSpeed)
{
    motorList[0] = &stepper0;
    motorList[1] = &stepper1;
    motorList[2] = &stepper2;
    motorList[3] = nullptr;
    doRotate(3, relSpeed);
}

template <unsigned p, unsigned u>
template <size_t N>
inline void StepControlBase<p, u>::rotateAsync(Stepper *(&motors)[N], float relSpeed)
{
    static_assert((N + 1) <= sizeof(motorList) / sizeof(motorList[0]), "Too many motors used, please increase MaxMotors");

    for (unsigned i = 0; i < N; i++)
    {
        motorList[i] = motors[i];
    }
    motorList[N] = nullptr;
    doRotate(N, relSpeed);
}

// STOP Commands -------------------------------------------------------------------------------------------------

template <unsigned p, unsigned u>
void StepControlBase<p, u>::stop()
{
    stopAsync();
    while (isRunning())
    {
        delay(10);
    }
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::stopAsync()
{
    uint32_t newTarget = initiateStopping(motorList[0]);
    motorList[0]->target = newTarget;
 }

// Protected -----------------------------------------------------------------------------------------------------

template <unsigned p, unsigned u>
void StepControlBase<p, u>::doMove(int N, float relSpeed, bool move)
{
    relSpeed = std::max(0.0, std::min(1.0, relSpeed)); // limit relative speed to [0..1]

    //Calculate Bresenham parameters ----------------------------------------------------------------
    std::sort(motorList, motorList + N, Stepper::cmpDelta); // The motor which does most steps leads the movement, move to top of list
    motorList[0]->current = 0;
    for (int i = 1; i < N; i++)
    {
        motorList[i]->current = 0;
        motorList[i]->leadTarget = motorList[0]->target;
        motorList[i]->D = 2 * motorList[i]->target - motorList[0]->target;
    }

    // //Calculate acceleration parameters --------------------------------------------------------------
    uint32_t pullInSpeed = (*std::min_element(motorList, motorList + N, Stepper::cmpVpullIn))->v_pullIn;                   // use the lowest pull in frequency for the move
    uint32_t targetSpeed = move ? (*std::min_element(motorList, motorList + N, Stepper::cmpV))->vMax * relSpeed : // use the lowest max frequency for the move, scale by relSpeed
                               (*std::max_element(motorList, motorList + N, Stepper::cmpV))->vMax * relSpeed;     // rotate: max frequency
    if (targetSpeed == 0)
        return;

    uint32_t targetPos = motorList[0]->target;
    if (targetPos == 0)
        return;

    uint32_t acceleration = (*std::min_element(motorList, motorList + N, Stepper::cmpAcc))->a; // use the lowest acceleration for the move

    uint32_t vstart = prepareMovement(targetPos, targetSpeed, pullInSpeed, acceleration);
   

    StepTimer.channel->LDVAL = F_BUS / vstart;

    // Start timers
    StepTimer.clearTIF();
    StepTimer.start();
    TeensyDelay::trigger(2, accLoopDelayChannel); // start the acceleration update loop
}

template <unsigned p, unsigned u>
void StepControlBase<p, u>::doRotate(int N, float relSpeed)
{
    uint32_t maxSpeed = (*std::max_element(motorList, motorList + N, Stepper::cmpV))->vMax;
    float fac = (float)std::numeric_limits<int32_t>::max() / 2.0 / maxSpeed;

    for (int i = 0; i < N; i++)
    {
        motorList[i]->setTargetRel(motorList[i]->vMax * fac * motorList[i]->direction);
    }

    doMove(N, relSpeed, false);
}
