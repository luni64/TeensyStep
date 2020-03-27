#include "Stepper.h"

#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-gpio.h"
#else
//#include "core_pins.h"
#endif

const int32_t Stepper::vMaxMax = 300000;   // largest speed possible (steps/s)
const uint32_t Stepper::aMax = 500000;     // speed up to 500kHz within 1 s (steps/s^2)
const uint32_t Stepper::vMaxDefault = 800; // should work with every motor (1 rev/sec in 1/4-step mode)
const uint32_t Stepper::aDefault = 2500;   // reasonably low (~0.5s for reaching the default speed)


Stepper::Stepper(const int _stepPin, const int _dirPin)
    : current(0),  stepPin(_stepPin), dirPin(_dirPin)
{
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);

    setStepPinPolarity(HIGH);
    setInverseRotation(false);
    setAcceleration(aDefault);
    setMaxSpeed(vMaxDefault);
}

Stepper &Stepper::setStepPinPolarity(int polarity)
{
    #ifdef TEENSY3
    // Calculate adresses of bitbanded pin-set and pin-clear registers
    uint32_t pinRegAddr = (uint32_t)digital_pin_to_info_PGM[stepPin].reg; //GPIO_PDOR
    uint32_t *pinSetReg = (uint32_t *)(pinRegAddr + 4 * 32);              //GPIO_PSOR = GPIO_PDOR + 4
    uint32_t *pinClearReg = (uint32_t *)(pinRegAddr + 8 * 32);            //GPIO_PCOR = GPIO_PDOR + 8
    // Assign registers according to step option
    if (polarity == LOW)
    {
        stepPinActiveReg = pinClearReg;
        stepPinInactiveReg = pinSetReg;
    }
    else
    {
        stepPinActiveReg = pinSetReg;
        stepPinInactiveReg = pinClearReg;
    }
    #else
    this->polarity = polarity;
    #endif
    clearStepPin(); // set step pin to inactive state
    return *this;
}

Stepper &Stepper::setInverseRotation(bool reverse)
{
    #ifdef TEENSY3
    // Calculate adresses of bitbanded pin-set and pin-clear registers
    uint32_t pinRegAddr = (uint32_t)digital_pin_to_info_PGM[dirPin].reg; //GPIO_PDOR
    uint32_t *pinSetReg = (uint32_t *)(pinRegAddr + 4 * 32);             //GPIO_PSOR = GPIO_PDOR + 4
    uint32_t *pinClearReg = (uint32_t *)(pinRegAddr + 8 * 32);           //GPIO_PCOR = GPIO_PDOR + 8

    if (reverse)
    {
        dirPinCwReg = pinClearReg;
        dirPinCcwReg = pinSetReg;
    }
    else
    {
        dirPinCwReg = pinSetReg;
        dirPinCcwReg = pinClearReg;
    }
    #else
    this->reverse = reverse;
    #endif
    return *this;
}

Stepper &Stepper::setAcceleration(uint32_t a) // steps/s^2
{
    this->a = std::min(aMax, a);
    return *this;
}

Stepper &Stepper::setMaxSpeed(int32_t speed)
{
    setDir(speed >= 0 ? 1 : -1);
    vMax = std::min(vMaxMax, std::max(-vMaxMax, speed));
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