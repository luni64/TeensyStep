#include "Stepper.h"
#include "core_pins.h"

namespace TeensyStep
{
    Stepper::Stepper(const int _stepPin, const int _dirPin)
        : current(0), stepPin(_stepPin), dirPin(_dirPin)
    {
        setStepPinPolarity(HIGH);
        setInverseRotation(false);
        setAcceleration(aDefault);
        setMaxSpeed(vMaxDefault);
        setPullInSpeed(vPullInOutDefault);

        pinMode(stepPin, OUTPUT);
        pinMode(dirPin, OUTPUT);
    }

    Stepper& Stepper::setStepPinPolarity(int polarity)
    {
        // Calculate adresses of bitbanded pin-set and pin-clear registers
        uint32_t pinRegAddr = (uint32_t)digital_pin_to_info_PGM[stepPin].reg; //GPIO_PDOR
        uint32_t* pinSetReg = (uint32_t*)(pinRegAddr + 4 * 32);               //GPIO_PSOR = GPIO_PDOR + 4
        uint32_t* pinClearReg = (uint32_t*)(pinRegAddr + 8 * 32);             //GPIO_PCOR = GPIO_PDOR + 8

        // Assign registers according to step option
        if (polarity == LOW)
        {
            stepPinActiveReg = pinClearReg;
            stepPinInactiveReg = pinSetReg;
        } else
        {
            stepPinActiveReg = pinSetReg;
            stepPinInactiveReg = pinClearReg;
        }
        clearStepPin(); // set step pin to inactive state
        return *this;
    }

    Stepper& Stepper::setInverseRotation(bool reverse)
    {
        // Calculate adresses of bitbanded pin-set and pin-clear registers
        uint32_t pinRegAddr = (uint32_t)digital_pin_to_info_PGM[dirPin].reg; //GPIO_PDOR
        uint32_t* pinSetReg = (uint32_t*)(pinRegAddr + 4 * 32);              //GPIO_PSOR = GPIO_PDOR + 4
        uint32_t* pinClearReg = (uint32_t*)(pinRegAddr + 8 * 32);            //GPIO_PCOR = GPIO_PDOR + 8

        if (reverse)
        {
            dirPinCwReg = pinClearReg;
            dirPinCcwReg = pinSetReg;
        } else
        {
            dirPinCwReg = pinSetReg;
            dirPinCcwReg = pinClearReg;
        }
        return *this;
    }

    Stepper& Stepper::setAcceleration(uint32_t a) // steps/s^2
    {
        this->a = std::min(aMax, a);
        return *this;
    }

    Stepper& Stepper::setMaxSpeed(int32_t speed)
    {
        setDir(speed >= 0 ? 1 : -1);
        vMax = std::min(vMaxMax, std::max(-vMaxMax, speed));
        return *this;
    }

    Stepper& Stepper::setPullInSpeed(int32_t speed)
    {
        vPullIn = vPullOut = std::abs(speed);
        return *this;
    }

    Stepper& Stepper::setPullInOutSpeed(int32_t pullInSpeed, int32_t pullOutSpeed)
    {
        vPullIn = std::abs(pullInSpeed);
        vPullOut = std::abs(pullOutSpeed);

        return *this;
    }

    void Stepper::setTargetAbs(int32_t target)
    {
        setTargetRel(target - current);
    }

    void Stepper::setTargetRel(int32_t delta)
    {
        setDir(delta < 0 ? -1 : 1);
        target = current + delta;
        A = std::abs(delta);
    }
}
