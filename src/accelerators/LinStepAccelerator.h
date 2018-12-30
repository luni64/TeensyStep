#pragma once

#include "wiring.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

class LinStepAccelerator
{
  public:    
    inline int32_t prepareMovement(int32_t currentPos, int32_t targetPos, uint32_t targetSpeed, uint32_t a);
    inline int32_t updateSpeed(int32_t currentPosition);
    inline int32_t initiateStopping(int32_t currentPosition);
    inline void overrideSpeed(float fac, int32_t currentPosition);

    LinStepAccelerator() = default;

  protected:
    LinStepAccelerator(const LinStepAccelerator &) = delete;
    LinStepAccelerator &operator=(const LinStepAccelerator &) = delete;

    int32_t s_0;
    uint32_t delta_tgt;
    uint32_t accEnd, decStart;
    uint32_t two_a;
    uint32_t v_tgt, v_min2;

    inline float signed_sqrt(int32_t x) // signed square root
    {
        return x > 0 ? sqrtf(x) : -sqrtf(-x);
    }
};


// Inline Implementation =====================================================================================================

int32_t LinStepAccelerator::prepareMovement(int32_t currentPos, int32_t targetPos, uint32_t targetSpeed, uint32_t a)
{
    s_0 = currentPos;
    delta_tgt = std::abs(targetPos - currentPos);
    v_tgt = targetSpeed;
    two_a = 2 * a;
    v_min2 = a;

    uint32_t ae = (float)v_tgt * v_tgt / two_a - 0.5f; // length of acceleration phase (we use a float here to avoid overflow in v_tgt^2). Use (1) and vmin^2 = 2a
    accEnd = std::min(ae, delta_tgt / 2);              // limit acceleration phase to half of total steps
    decStart = delta_tgt - accEnd;

    Serial.printf("ae: %i ds:%i\n", accEnd, decStart);
    return accEnd == 0 ? v_tgt : (uint32_t)sqrtf(v_min2);
}

int32_t LinStepAccelerator::updateSpeed(int32_t curPos)
{
    uint32_t delta = std::abs(s_0 - curPos);

    // acceleration phase -------------------------------------
    if (delta < accEnd)
       return sqrtf(two_a * delta + v_min2);       

    // constant speed phase ------------------------------------
    if (delta < decStart)
        return v_tgt;

    //deceleration phase --------------------------------------
    return sqrtf(two_a * ((delta < delta_tgt - 1) ? delta_tgt - delta - 2 : 0) + v_min2);
}

int32_t LinStepAccelerator::initiateStopping(int32_t curPos)
{
    uint32_t delta = std::abs(delta_tgt - curPos);

    if (delta <= decStart) // we are already decelerationg, nothing to change...
    {
        return delta_tgt;
    }
    else // accelerating or constant speed phase
    {
        uint32_t newTarget;
        if (delta < accEnd)        // still accelerating
            newTarget = delta * 2; // will take the same distance to stop as we already traveled
        else                       // constant speed phase
            newTarget = delta + accEnd;

        accEnd = 0;
        decStart = delta; // start decelerating now
        return newTarget;
    }
}