#pragma once

// Including main.h ensures that the correct HAL headers are included.
// STM32Step assumes the use of STM32 HAL environment.
#include "main.h"

#include <cstdint>
#include <algorithm>

/**
 * Helper struct for the representation of GPIO pins
 * */
struct GpioPin{
    GPIO_TypeDef* port;
    uint16_t pin;
};
#define REVERSE_POLARITY(polarity) ((!(polarity)) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define GPIO_PIN(name) (GpioPin{name##_GPIO_Port,name##_Pin}) 

/**
 * GPIOStepper represents a stepper motor driving port and handles the GPIO based signal generation.
 * */
class GPIOStepper
{
    static constexpr int32_t vMaxMax = 300000;   // largest speed possible (steps/s)
    static constexpr uint32_t aMax = 500000;     // speed up to 500kHz within 1 s (steps/s^2)
    static constexpr uint32_t vMaxDefault = 800; // should work with every motor (1 rev/sec in 1/4-step mode)
    static constexpr uint32_t aDefault = 2500;   // reasonably low (~0.5s for reaching the default speed)

  public:
    GPIOStepper(const GpioPin StepPin, const GpioPin DirPin);

    GPIOStepper &setMaxSpeed(int32_t speed);   // steps/s
    GPIOStepper &setAcceleration(uint32_t _a); // steps/s^2

    GPIOStepper &setStepPinPolarity(GPIO_PinState p);  // HIGH -> positive pulses, LOW -> negative pulses
    GPIOStepper &setDirectionPolarity(GPIO_PinState p); // Set pin state corresponding to positive direction

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
    static bool cmpDelta(const GPIOStepper *a, const GPIOStepper *b) { return a->A > b->A; }
    static bool cmpAcc(const GPIOStepper *a, const GPIOStepper *b) { return a->a < b->a; }
    static bool cmpVmin(const GPIOStepper *a, const GPIOStepper *b) { return std::abs(a->vMax) < std::abs(b->vMax); }
    static bool cmpVmax(const GPIOStepper *a, const GPIOStepper *b) { return std::abs(a->vMax) > std::abs(b->vMax); }

    // Pin & Dir registers
    const GpioPin stepPin, dirPin;
    GPIO_PinState stepPinPolarity, directionPolarity;

    // Friends
    template <typename a, typename t>
    friend class PositionControlBase;

    template <typename a, typename t>
    friend class SpeedControlBase;

    friend class GPIOStepControl;
};

// Inline implementation -----------------------------------------

void GPIOStepper::doStep()
{
    HAL_GPIO_WritePin(stepPin.port, stepPin.pin, stepPinPolarity);
    current += dir;
}
void GPIOStepper::clearStepPin() const
{
    HAL_GPIO_WritePin(stepPin.port, stepPin.pin, REVERSE_POLARITY(stepPinPolarity));
}

void GPIOStepper::setDir(int8_t d)
{
    dir = d;
    HAL_GPIO_WritePin(dirPin.port, dirPin.pin, dir == 1 ? directionPolarity : REVERSE_POLARITY(directionPolarity));
}

void GPIOStepper::toggleDir()
{
    setDir(-dir);
}
