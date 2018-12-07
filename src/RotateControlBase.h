#pragma once

#include "MotorControlBase.h"
#include "Stepper.h"
#include <algorithm>

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControlBase : public MotorControlBase
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
    RotateControlBase(const RotateControlBase &) = delete;
    RotateControlBase &operator=(const RotateControlBase &) = delete;

    int32_t virtual updateSpeed(uint32_t currentPosition) = 0;
    uint32_t virtual updateSpeed2(Stepper *stepper) = 0;
    uint32_t virtual initiateStopping(uint32_t currentPosition) = 0;
    uint32_t virtual prepareRotation(uint32_t currentPosition, int32_t targetSpeed, uint32_t pullInSpeed, uint32_t acceleration) = 0;

    void doRotate(int N);
    void doMove(int N, bool mode = true);

    void pitISR();
    void delayISR(unsigned channel);

    void (*callback)() = nullptr;
};

// Implementation *************************************************************************************************

template <unsigned p, unsigned u>
RotateControlBase<p, u>::RotateControlBase() : MotorControlBase()
{
}

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
            (*motor)->D -= (*motor)->leadTarget; // do not use motorlist[0]->target since this will be changed by stop()
        }
        (*motor)->D += (*motor)->target;
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

    // calculate new speed (i.e., timer reload value) -------------------
    if (channel == accLoopDelayChannel)
    {
        static int oldspeed = -1;
        cli();
        TeensyDelay::trigger(u, accLoopDelayChannel); // retrigger
        sei();

        // if (StepTimer.isRunning())
        {
            digitalWriteFast(2, HIGH);

            int32_t newSpeed = updateSpeed(motorList[0]->current);
            //uint32_t newSpeed = updateSpeed2(motorList[0]);
            Serial.printf("%d\t%d\n",motorList[0]->current, newSpeed);
            if (newSpeed != 0)
            {
                if (oldspeed != newSpeed)
                {
                    StepTimer.channel->LDVAL = F_BUS / newSpeed;
                    if (oldspeed == 0)
                    {
                        StepTimer.clearTIF();
                        StepTimer.start();
                    }
                    // TeensyDelay::trigger(2, accLoopDelayChannel); // start the acceleration update loop
                    oldspeed = newSpeed;
                }
            }
            else
            {
                //  Serial.println("stop");
                StepTimer.channel->TCTRL &= ~PIT_TCTRL_TIE; // disable timer interrupts
            }
            digitalWriteFast(2, LOW);
        }
    }
}

// ROTATE Commands -----------------------------------------------------------------------------------------------

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

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::stopAsync()
{
    initiateStopping(motorList[0]->current);
    // motorList[0]->target = newTarget;
}

// Protected -----------------------------------------------------------------------------------------------------

template <unsigned p, unsigned u>
void RotateControlBase<p, u>::doMove(int N, bool move)
{
    //Calculate Bresenham parameters ----------------------------------------------------------------
    std::sort(motorList, motorList + N, Stepper::cmpDelta); // The motor which does most steps leads the movement, move to top of list
    motorList[0]->current = 0;
    for (int i = 1; i < N; i++)
    {
        motorList[i]->current = 0;
        motorList[i]->leadTarget = motorList[0]->target;
        motorList[i]->D = 2 * motorList[i]->target - motorList[0]->target;
    }

    // Rotation parameters --------------------------------------------------------------
    uint32_t acceleration = (*std::min_element(motorList, motorList + N, Stepper::cmpAcc))->a;           // use the lowest acceleration for the move
    uint32_t pullInSpeed = (*std::min_element(motorList, motorList + N, Stepper::cmpVpullIn))->v_pullIn; // use the lowest pull in frequency for the move
    uint32_t targetSpeed = (*std::max_element(motorList, motorList + N, Stepper::cmpV))->vMax;           // use the highest vMax for the move
    if (targetSpeed == 0)
        return;

    uint32_t vstart = prepareRotation(motorList[0]->position, targetSpeed, pullInSpeed, acceleration);

    StepTimer.channel->LDVAL = F_BUS / vstart;

    //Serial.println(StepTimer.channel->LDVAL);

    // Start timers
    StepTimer.clearTIF();
    StepTimer.start();
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
