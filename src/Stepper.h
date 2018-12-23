#pragma once

#include <cstdint>
#include <algorithm>

class Stepper
{
    static constexpr int32_t vMaxMax = 300000;   // largest speed possible (steps/s)
    static constexpr uint32_t aMax = 500000;     // speed up to 500kHz within 1 s (steps/s^2)    
    static constexpr uint32_t vMaxDefault = 800; // should work with every motor (1 rev/sec in 1/4-step mode)
    static constexpr uint32_t aDefault = 2500;   // reasonably low (~0.5s for reaching the default speed)

  public:
    Stepper(const int StepPin, const int DirPin);

    Stepper &setMaxSpeed(int32_t speed);         // steps/s
    Stepper &setAcceleration(uint32_t _a);       // steps/s^2

    Stepper &setStepPinPolarity(int p);          // HIGH -> positive pulses, LOW -> negative pulses
    Stepper &setInverseRotation(bool b);         // Change polarity of the dir pulse 

    void setTargetAbs(int32_t pos);              // Set target position absolute
    void setTargetRel(int32_t delta);            // Set target position relative to current position

    inline int32_t getPosition() const { return current; }
    inline void setPosition(int32_t pos) { current = pos; }

  protected:
    inline void doStep();
    inline void clearStepPin() const;

    inline void setDir(int d);
    inline void toggleDir();

    volatile int32_t current;
    volatile int32_t currentSpeed;
    volatile int32_t target;

    int32_t  distance, B; // Bresenham paramters
    int32_t vMax;
    uint32_t a;
    int32_t dir;

    // compare functions
    static bool cmpDelta(const Stepper *a, const Stepper *b) { return a->distance > b->distance; }
    static bool cmpAcc(const Stepper *a, const Stepper *b) { return a->a < b->a; }    
    static bool cmpV(const Stepper *a, const Stepper *b) { return std::abs(a->vMax) < std::abs(b->vMax); }

    // Pin & Dir registers
    volatile uint32_t *stepPinActiveReg;
    volatile uint32_t *stepPinInactiveReg;
    volatile uint32_t *dirPinCwReg;
    volatile uint32_t *dirPinCcwReg;
    const int stepPin, dirPin;

    // Friends
    template <unsigned u, unsigned p>
    friend class StepControlBase;

    template <unsigned u, unsigned p>
    friend class RotateControlBase;
};

// Inline implementation -----------------------------------------

void Stepper::doStep()
{
    *stepPinActiveReg = 1;
    current += dir;
}
void Stepper::clearStepPin() const
{
    *stepPinInactiveReg = 1;
}

void Stepper::setDir(int d)
{
    dir = d;
    if (dir == 1)
    {
        *dirPinCwReg = 1;
    }
    else
    {
        *dirPinCcwReg = 1;
    }
}

void Stepper::toggleDir()
{
    setDir(-dir);
}
