#pragma once 

#include "./PIT/PIT.h"
#include "./TeensyDelay/TeensyDelay.h"
#include "Stepper.h"
#include <algorithm>
//#include "arduino.h"

//unsigned short isqrt(unsigned long a);

constexpr int MaxMotors = 10;
constexpr uint64_t sqr(uint32_t x) { return (uint64_t)x*(uint64_t)x; } 

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
	bool isRunning() { return StepTimer.channel->TCTRL & PIT_TCTRL_TIE; }

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

	void emergencyStop()
	{
		StepTimer.disableInterupt();
	}

	void stopAsync()
	{
		int current = motorList[0]->current;
		int target = motorList[0]->target;

		if (current > decelerationStart) return;   // already decelerating -> do nothing

		unsigned newTarget = current < accelerationEnd ? current * 2 : current + accelerationEnd;
		float changeFactor = (float)newTarget / target;

		Stepper** motor = motorList;

		cli();
		(*motor)->target = newTarget;         // set new target for fast axis
		while (*(++motor) != nullptr)         // adjust targets of slow axes to keep frequency ratio
		{
			(*motor)->target *= changeFactor;
		}
		accelerationEnd = 0;
		decelerationStart = current;          // start decelerating now
		sei();
	}

	void stop()
	{
		stopAsync();
		while (isRunning())
		{
			delay(10);
		}
	}


protected:

	//*************************************************************************************************************
	template<int N>
	void doMove(float relSpeed)
	{
		relSpeed = std::max(0.0, std::min(1.0, relSpeed)); // limit relative speed to [0..1]

		//Calculate Bresenham parameters ---------------------------------------------------------------- 
		std::sort(motorList, motorList + N, Stepper::cmpDelta);	// The motor which does most steps leads the movement, move to top of list																
		motorList[0]->current = 0;
		for (int i = 1; i < N; i++)
		{
			motorList[i]->current = 0;
			motorList[i]->D = 2 * motorList[i]->target - motorList[0]->target;
		}

		//Calculate acceleration parameters --------------------------------------------------------------
		vMin = (*std::min_element(motorList, motorList + N, Stepper::cmpVpullIn))->v_pullIn;	        // use the lowest pull in frequency for the move
		uint32_t v = (*std::min_element(motorList, motorList + N, Stepper::cmpV))->vMax * relSpeed;	    // use the lowest max frequency for the move, scale by relSpeed
		uint32_t a = (*std::min_element(motorList, motorList + N, Stepper::cmpAcc))->a;					// use the lowest acceleration for the move
		if (v == 0) return;

		uint32_t target = motorList[0]->target;

		if (v > vMin) // acceleration required
		{			
			float dv = v - vMin;
			float ae = dv * dv / (2.0f * a);  // length of acceleration phase
			
			if (ae > std::numeric_limits<uint32_t>::max()) return;

			accelerationEnd = std::min((uint32_t)ae, target / 2);
			decelerationStart = target - accelerationEnd;
					

			cMax = (F_BUS / v);
			sqrt_2a = sqrtf(a * 2);
			StepTimer.channel->LDVAL = F_BUS / vMin;
		}
		else  // constant velocity, no acceleration necessary
		{
			accelerationEnd = 0;
			decelerationStart = target;
			StepTimer.channel->LDVAL = F_BUS / v;
		}

		pitISR();                                       // immediately make first step 

		// Start timers
		StepTimer.clearTIF();
		StepTimer.start();
		TeensyDelay::trigger(2, accLoopDelayChannel);   // start the acceleration update loop 
	}

	//*************************************************************************************************************
	void pitISR()
	{
		Stepper** motor = motorList;   // move leading axis	
		(*motor)->doStep();            // activate step pin 

		while (*(++motor) != nullptr)  // move slow axes if required (https://en.wikipedia.org/wiki/Bresenham)
		{
			if ((*motor)->D >= 0)
			{
				(*motor)->doStep();
				(*motor)->D -= motorList[0]->target;
			}
			(*motor)->D += (*motor)->target;
		}
		TeensyDelay::trigger(pulseWidth, pinResetDelayChannel); // start delay line to dactivate all step pins  

		if (motorList[0]->current >= motorList[0]->target)      // stop if we are at target
		{
			StepTimer.channel->TCTRL &= ~PIT_TCTRL_TIE;         // disable timer interrupts
		}
	}

	//*************************************************************************************************************
	void delayISR(unsigned channel)
	{
		if (channel == pinResetDelayChannel)  //clear all step pins
		{
			Stepper** motor = motorList;
			while ((*motor) != nullptr) {
				(*motor++)->clearStepPin();
			}
		}

		if (channel == accLoopDelayChannel) // calculate new speed (i.e., timer reload value)
		{
			if (StepTimer.isRunning())
			{
				cli();
				TeensyDelay::trigger(accUpdatePeriod, accLoopDelayChannel); // retrigger          
				sei();

				uint32_t pos = motorList[0]->current;

				if (pos < accelerationEnd)       // accelerating	
				{					
					StepTimer.channel->LDVAL = F_BUS / (sqrt_2a * sqrtf(pos) + vMin);
				}
				else if (pos < decelerationStart) // constant speed			 
				{				
					StepTimer.channel->LDVAL = cMax;
				}
				else						      //decelerating	
				{				
					StepTimer.channel->LDVAL = F_BUS / (sqrt_2a * sqrtf(motorList[0]->target - pos - 1) + vMin);
				}
			}
		}
	}


	bool OK = false;
	PIT StepTimer;
	uint32_t accelerationEnd;
	uint32_t decelerationStart;
	uint32_t cMax;
	uint32_t vMin;
	uint32_t sqrt_2a;
	const uint pinResetDelayChannel;
	const uint accLoopDelayChannel;
	Stepper* motorList[MaxMotors];
};
