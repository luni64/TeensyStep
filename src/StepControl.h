#pragma once 

#include "./PIT/Pit.h"
#include "./TeensyDelay/TeensyDelay.h"
#include "Stepper.h"
#include <algorithm>


constexpr int MaxMotors = 10;
constexpr uint32_t sqr(uint32_t x) { return x*x; }

template<const uint pulseWidth = 5, uint accUpdatePeriod = 5000>
class StepControl : IPitHandler, IDelayHandler
{
public:
    StepControl()
        : pinResetDelayChannel(TeensyDelay::addDelayChannel(this)),
        accLoopDelayChannel(TeensyDelay::addDelayChannel(this))
    {
        OK = StepTimer.begin(this);
        TeensyDelay::begin();
    }

    bool isOk() { return OK; }
    bool isRunning() { return StepTimer.isRunning(); }

    void move(Stepper& stepper, float relSpeed = 1)
    {
        moveAsync(stepper, relSpeed);
        while (isRunning())
            delay(10);
    }

    void move(Stepper& stepper1, Stepper& stepper2, float relSpeed = 1)
    {
        moveAsync(stepper1, stepper2, relSpeed);
        while (isRunning())
            delay(10);
    }

    void move(Stepper& stepper1, Stepper& stepper2, Stepper& stepper3, float relSpeed = 1)
    {
        moveAsync(stepper1, stepper2, stepper3, relSpeed);
        while (isRunning())
            delay(10);
    }

    void moveAsync(Stepper& stepper, float relSpeed = 1)
    {
        motorList[0] = &stepper; motorList[1] = nullptr;
        doMove<1>(relSpeed);
    }

    void moveAsync(Stepper& stepper0, Stepper& stepper1, float relSpeed = 1)
    {
        motorList[0] = &stepper0;	motorList[1] = &stepper1;	motorList[2] = nullptr;
        doMove<2>(relSpeed);
    }

    void moveAsync(Stepper& stepper0, Stepper& stepper1, Stepper& stepper2, float relSpeed = 1)
    {
        motorList[0] = &stepper0;	motorList[1] = &stepper1;	motorList[2] = &stepper2;	motorList[3] = nullptr;
        doMove<3>(relSpeed);
    }

    template<size_t N>
    void moveAsync(Stepper* (&motors)[N], float relSpeed = 1)  //move up to maxMotors motors synchronously
    {
        static_assert((N + 1) <= sizeof(motorList) / sizeof(motorList[0]), "Too many motors used, please increase MaxMotors");

        for (int i = 0; i < N; i++) {
            motorList[i] = motors[i];
        }
        motorList[N] = nullptr;
        doMove<N>(relSpeed);
    }

protected:
    PIT StepTimer;

    void pitISR()
    {
        //Move leading axis	
        Stepper** motor = motorList;
        (*motor)->setStepPin();
        (*motor)->current++;

        //Move slow axes if required (Bresenham algorithmus "https://en.wikipedia.org/wiki/Bresenham's_line_algorithm")
        while (*(++motor) != nullptr)
        {
            if ((*motor)->D >= 0)
            {
                (*motor)->setStepPin();
                (*motor)->current++;
                (*motor)->D -= motorList[0]->target;
            }
            (*motor)->D += (*motor)->target;
        }
        TeensyDelay::trigger(pulseWidth, pinResetDelayChannel); // start delay line to reset step pins  

        if (motorList[0]->current < motorList[0]->target) {
            StepTimer.setReloadValue(AccParams.nextCnt);
        }
        else {
            StepTimer.disableInterupt();
        }
    }

    static constexpr int pitCor = 0; //Takes 24 cycles to enter the ISR this needs to be subtracted from the reload value; Optimized with -O2, 

    void delayISR(unsigned channel)
    {
        if (channel == pinResetDelayChannel)  //clear all step pins
        {
            Stepper** motor = motorList;
            while ((*motor) != nullptr) {
                (*motor++)->clearStepPin();
            }
        }
        else //accLoopDelayChannel -> calculate new value for the step timer reload value
        {
            if (StepTimer.isRunning())
            {
                TeensyDelay::trigger(accUpdatePeriod, accLoopDelayChannel); // retrigger          
                int pos = motorList[0]->current;
                if (pos < AccParams.s_acc) { // accelerating				
                    AccParams.nextCnt = F_BUS / sqrtf(2 * AccParams.a * pos + AccParams.v0_sqr) - pitCor;
                }
                else if (pos < AccParams.s_dec) { // constant speed			
                    AccParams.nextCnt = AccParams.cEnd;
                }
                else { //decelerating				
                    AccParams.nextCnt = F_BUS / sqrtf(2 * AccParams.a *(motorList[0]->target - pos - 2) + AccParams.v0_sqr) - pitCor;
                }
            }
        }
    }

    template<int N>
    void doMove(float relSpeed)
    {
        relSpeed = std::max(0.0, std::min(1.0, relSpeed));

        ////Calculate Bresenham parameters ---------------------------------------------------------------- 
        std::sort(motorList, motorList + N, Stepper::cmpDelta);	// The motor which does most steps leads the movement
                                                                // Move this motor on top of the list
        motorList[0]->current = 0;
        for (int i = 1; i < N; i++)
        {
            motorList[i]->current = 0;
            motorList[i]->D = 2 * motorList[i]->target - motorList[0]->target;
        }

        //Calculate acceleration parameters
        uint32_t vMin = (*std::min_element(motorList, motorList + N, Stepper::cmpVpullIn))->v_pullIn;	// use the lowest pull in frequency for the move
        uint32_t v = (*std::min_element(motorList, motorList + N, Stepper::cmpV))->vMax * relSpeed;	    // use the lowest max frequency for the move, scale by relSpeed
        uint32_t a = (*std::min_element(motorList, motorList + N, Stepper::cmpAcc))->a;					// use the lowest acceleration for the move

        if (v == 0) return;
        if (v < vMin) {
            AccParams.cEnd = (F_BUS / v) - pitCor;
            AccParams.nextCnt = AccParams.cEnd;
            AccParams.s_acc = 0;
            AccParams.s_dec = motorList[0]->target;
        }
        else {
            AccParams.cEnd = (F_BUS / v) - pitCor;
            AccParams.a = a;
            AccParams.nextCnt = F_BUS / vMin - pitCor;
            AccParams.v0_sqr = sqr(vMin);
            AccParams.s_acc = sqr(v - vMin) / 2 / a;
            AccParams.s_dec = motorList[0]->target - AccParams.s_acc;
            if (AccParams.s_dec < AccParams.s_acc) {
                AccParams.s_acc = AccParams.s_dec = motorList[0]->target / 2;   // distance too short to reach end speed
            }
        }

        // Start timers
        pitISR();                                       // call to pitISR makes first stepk, sets reload value and starts timer                     															 
        TeensyDelay::trigger(2, accLoopDelayChannel);   // start the acceleration update loop after first step was done
    }

    struct {
        volatile uint32_t nextCnt;
        uint32_t cEnd;
        uint32_t a;
        uint32_t v0_sqr;
        int32_t s_acc;      // end of acceleration (steps)
        int32_t s_dec;      // start of decceleration (steps)
    } AccParams;

    bool OK = false;
    const uint pinResetDelayChannel;
    const uint accLoopDelayChannel;
    Stepper* motorList[MaxMotors];
};

