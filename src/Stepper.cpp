#include "Stepper.h" 
#include "core_pins.h"
#include <math.h>
#include <algorithm>

#include <HardwareSerial.h>



Stepper::Stepper(const int stepPin, const int dirPin, bool reverseDir, bool stepsActiveLow)
	: current(0), v_pullIn(vPullIn_default), vMax(vMaxDefault), /*v(vDefault),*/ a(aDefault), position(0)	
{
	pinMode(stepPin, OUTPUT);
	pinMode(dirPin, OUTPUT);       

	
    //==================================================================================================
	// We use bitBanding to set/reset the step and dir pins. 	
	//
	// Step Pin ----------------------------------------------------------------------------------------
	// Calculate adresses of bitbanded pin-set and pin-clear registers
	uint32_t  pinRegAddr = (uint32_t)digital_pin_to_info_PGM[stepPin].reg;	//GPIO_PDOR
	uint32_t* pinSetReg = (uint32_t*)(pinRegAddr + 4 * 32);					//GPIO_PSOR = GPIO_PDOR + 4
	uint32_t* pinClearReg = (uint32_t*)(pinRegAddr + 8 * 32);				//GPIO_PCOR = GPIO_PDOR + 8

	// Assign registers according to step option
	if (stepsActiveLow) {
		stepPinActiveReg = pinClearReg;
		stepPinInactiveReg = pinSetReg;
	}
	else {
		stepPinActiveReg = pinSetReg;
		stepPinInactiveReg = pinClearReg;
	}
    clearStepPin(); // set step pin to inactive state


	// Dir pin -----------------------------------------------------------------------------------------
	// Calculate adresses of bitbanded pin-set and pin-clear registers
	pinRegAddr = (uint32_t)digital_pin_to_info_PGM[dirPin].reg;	//GPIO_PDOR
	pinSetReg = (uint32_t*)(pinRegAddr + 4 * 32);				//GPIO_PSOR = GPIO_PDOR + 4
	pinClearReg = (uint32_t*)(pinRegAddr + 8 * 32);				//GPIO_PCOR = GPIO_PDOR + 8

	// Assign registers according to DIR option
	if (reverseDir) {
		dirPinCwReg = pinClearReg;
		dirPinCcwReg = pinSetReg;
	}
	else {
		dirPinCwReg = pinSetReg;
		dirPinCcwReg = pinClearReg;
	}
}


void Stepper::SetTargetAbs(int target)
{
    position += dirCw *current;	// update position from last move;
    current = 0; 
    int delta = target - position;

    if (delta >= 0) {
        dirCw = 1;
        target = delta;
        *dirPinCwReg = 1;
    }
    else {
        dirCw = -1;
        target = -delta;
        *dirPinCcwReg = 1;
    }	
}

void Stepper::SetTargetRel(int delta)
{  
	position += dirCw *current;	// update position from last move;
	current = 0;

	if (delta >= 0) {
		dirCw = 1;
		target = delta;
		*dirPinCwReg = 1;
	}
	else {
		dirCw = -1;
		target = -delta;
		*dirPinCcwReg = 1;
	}
}


