#pragma once


#include "Arduino.h"


class LinRotAccelerator
{
public:
    inline void prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration, uint32_t accUpdatePeriod, float speedFactor = 1.0);
    inline int32_t updateSpeed(int32_t currentPosition);
    inline int32_t initiateStopping(int32_t currentPosition);
    inline void overrideSpeed(float factor);
    inline void overrideAcceleration(float factor);

    LinRotAccelerator() = default;

protected:
    LinRotAccelerator(const LinRotAccelerator &) = delete;
    LinRotAccelerator &operator=(const LinRotAccelerator &) = delete;

    // int32_t v_tgt, v_cur;
    // int32_t v_tgt_orig, dv_orig, dv;

    float v_tgt, v_cur;
    float v_tgt_orig, dv_orig, dv_cur, dv;
};

// Inline Implementation =====================================================================================================

void LinRotAccelerator::prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t a, uint32_t accUpdatePeriod, float speedFactor)
{
    v_tgt_orig = targetSpeed;
    dv_orig = ((float)a * accUpdatePeriod) / 1E6;
    v_cur = 0;

    overrideAcceleration(1.0f);
    overrideSpeed(speedFactor);
}

void LinRotAccelerator::overrideSpeed(float factor)
{
    //Serial.printf("a:------ %d\n", a);

    noInterrupts();
    v_tgt = v_tgt_orig * factor;
    dv = v_tgt > v_cur ? dv_cur : -dv_cur;
    interrupts();
}

void LinRotAccelerator::overrideAcceleration(float factor)
{
    //Serial.printf("a:------ %d\n", a);
    if (factor > 0)
    {
        noInterrupts();
        dv_cur = dv_orig * factor;
        dv *= factor;
        interrupts();
    }
}

int32_t LinRotAccelerator::updateSpeed(int32_t curPos)
{
    if (v_cur == v_tgt)
        return (int32_t)v_tgt; // already at target, keep spinning with target frequency

    v_cur += dv;
    v_cur = dv > 0.0f ? std::min(v_tgt, v_cur) : std::max(v_tgt, v_cur);

    return (int32_t)v_cur;
}

int32_t LinRotAccelerator::initiateStopping(int32_t curPos)
{
    overrideSpeed(0);
    return 0;
}

// void LinRotAccelerator::eStop()
// {
//     noInterrupts();
//     v_cur = 0.0f;
//     v_tgt = 0.0f;
//     interrupts();
// }

