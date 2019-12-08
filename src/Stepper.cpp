#include "Stepper.h"

constexpr uint32_t Stepper::aMax;
constexpr int32_t Stepper::vMaxMax;

Stepper::Stepper(const GpioPin _stepPin, const GpioPin _dirPin)
    : current(0),  stepPin(_stepPin), dirPin(_dirPin)
{
    setStepPinPolarity(GPIO_PIN_SET);
    setDirectionPolarity(GPIO_PIN_SET);
    setAcceleration(aDefault);
    setMaxSpeed(vMaxDefault);
    
    // TODO: proper setup for output pins (or let the user do it in CubeMX)
    //pinMode(stepPin, OUTPUT);
    //pinMode(dirPin, OUTPUT);
}

Stepper &Stepper::setStepPinPolarity(GPIO_PinState polarity)
{
    stepPinPolarity = polarity;
    clearStepPin(); // set step pin to inactive state
    return *this;
}

Stepper &Stepper::setDirectionPolarity(GPIO_PinState polarity)
{
    directionPolarity = polarity;
    setDir(dir); // update direction pins state
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