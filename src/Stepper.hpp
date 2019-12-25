#pragma once

// Including main.h ensures that the correct HAL headers are included.
// STM32Step assumes the use of STM32Cube environment.
extern "C" {
#include "main.h"
}

#include <cstdint>
#include <algorithm>

/**
 * Helper struct for communicating GPIO pins
 * */
struct GpioPin{
    GPIO_TypeDef* port;
    uint16_t pin;
};
#define REVERSE_POLARITY(polarity) ((!(polarity)) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define GPIO_PIN(name) (GpioPin{name##_GPIO_Port,name##_Pin}) 


class Stepper
{
    static constexpr int32_t vMaxMax = 300000;   // largest speed possible (steps/s)
    static constexpr uint32_t aMax = 500000;     // speed up to 500kHz within 1 s (steps/s^2)
    static constexpr uint32_t vMaxDefault = 800; // should work with every motor (1 rev/sec in 1/4-step mode)
    static constexpr uint32_t aDefault = 2500;   // reasonably low (~0.5s for reaching the default speed)

  public:
    Stepper(const GpioPin StepPin, const GpioPin DirPin);

    Stepper &setMaxSpeed(int32_t speed);   // steps/s
    Stepper &setAcceleration(uint32_t _a); // steps/s^2

    Stepper &setStepPinPolarity(GPIO_PinState p);  // HIGH -> positive pulses, LOW -> negative pulses
    Stepper &setDirectionPolarity(GPIO_PinState p); // Set pin state corresponding to positive direction

    void setTargetAbs(int32_t pos);   // Set target position absolute
    void setTargetRel(int32_t delta); // Set target position relative to current position

    inline int32_t getPosition() const { return current; }
    inline void setPosition(int32_t pos) { current = pos; }
    int8_t dir;

  protected:
    inline void doStep();
    inline void clearStepPin() const;

    inline void setDir(int8_t d);
    inline void toggleDir();

    volatile int32_t current;
    volatile int32_t currentSpeed; 
    volatile int32_t target;

    int32_t A, B; // Bresenham paramters
    int32_t vMax;
    uint32_t a;

    // compare functions
    static bool cmpDelta(const Stepper *a, const Stepper *b) { return a->A > b->A; }
    static bool cmpAcc(const Stepper *a, const Stepper *b) { return a->a < b->a; }
    static bool cmpVmin(const Stepper *a, const Stepper *b) { return std::abs(a->vMax) < std::abs(b->vMax); }
    static bool cmpVmax(const Stepper *a, const Stepper *b) { return std::abs(a->vMax) > std::abs(b->vMax); }

    // Pin & Dir registers
    const GpioPin stepPin, dirPin;
    GPIO_PinState stepPinPolarity, directionPolarity;

    // Friends
    template <typename a, typename t>
    friend class StepControlBase;

    template <typename a, typename t>
    friend class RotateControlBase;

    template <typename t>
    friend class MotorControlBase;
};

// Inline implementation -----------------------------------------

void Stepper::doStep()
{
    HAL_GPIO_WritePin(stepPin.port, stepPin.pin, stepPinPolarity);
    current += dir;
}
void Stepper::clearStepPin() const
{
    HAL_GPIO_WritePin(stepPin.port, stepPin.pin, REVERSE_POLARITY(stepPinPolarity));
}

void Stepper::setDir(int8_t d)
{
    dir = d;
    HAL_GPIO_WritePin(dirPin.port, dirPin.pin, dir == 1 ? directionPolarity : REVERSE_POLARITY(directionPolarity));
}

void Stepper::toggleDir()
{
    setDir(-dir);
}
