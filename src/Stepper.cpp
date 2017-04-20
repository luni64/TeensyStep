#include "Stepper.h" 
#include "core_pins.h"

Stepper::Stepper(const int _stepPin, const int _dirPin) 
    : current(0), v_pullIn(vPullIn_default), vMax(vMaxDefault), a(aDefault), position(0), stepPin(_stepPin), dirPin(_dirPin)
{
    setStepPinPolarity(HIGH);
    setInverseRotation(false);

    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
}

Stepper& Stepper::setStepPinPolarity(int polarity)
{ 
    // Calculate adresses of bitbanded pin-set and pin-clear registers
    uint32_t  pinRegAddr = (uint32_t)digital_pin_to_info_PGM[stepPin].reg;  //GPIO_PDOR
    uint32_t* pinSetReg = (uint32_t*)(pinRegAddr + 4 * 32);                 //GPIO_PSOR = GPIO_PDOR + 4
    uint32_t* pinClearReg = (uint32_t*)(pinRegAddr + 8 * 32);               //GPIO_PCOR = GPIO_PDOR + 8

                                                                            // Assign registers according to step option
    if (polarity == LOW) {
        stepPinActiveReg = pinClearReg;
        stepPinInactiveReg = pinSetReg;
    }
    else {
        stepPinActiveReg = pinSetReg;
        stepPinInactiveReg = pinClearReg;
    }
    clearStepPin(); // set step pin to inactive state
    return *this;
}

Stepper& Stepper::setInverseRotation(bool reverse)
{    
    // Calculate adresses of bitbanded pin-set and pin-clear registers
    uint32_t pinRegAddr = (uint32_t)digital_pin_to_info_PGM[dirPin].reg;    //GPIO_PDOR
    uint32_t* pinSetReg = (uint32_t*)(pinRegAddr + 4 * 32);                 //GPIO_PSOR = GPIO_PDOR + 4
    uint32_t* pinClearReg = (uint32_t*)(pinRegAddr + 8 * 32);               //GPIO_PCOR = GPIO_PDOR + 8
                                                            
    if (reverse) {
        dirPinCwReg = pinClearReg;
        dirPinCcwReg = pinSetReg;
    }
    else {
        dirPinCwReg = pinSetReg;
        dirPinCcwReg = pinClearReg;
    }
    return *this;
}

void Stepper::setTargetAbs(int _target)
{
    position += dirCw *current;	// update position from last move;
    current = 0; 
    int delta = _target - position;

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

void Stepper::setTargetRel(int delta)
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