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
    inline uint32_t initiateStopping(int32_t currentPosition);
    inline void overrideSpeed(float fac, int32_t currentPosition);

    LinStepAccelerator() = default;

  protected:
    LinStepAccelerator(const LinStepAccelerator &) = delete;
    LinStepAccelerator &operator=(const LinStepAccelerator &) = delete;

    int32_t s_0;
    uint32_t delta_tgt;
    uint32_t accLength, decStart;
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
    accLength = std::min(ae, delta_tgt / 2);           // limit acceleration phase to half of total steps
    decStart = delta_tgt - accLength;
    
    return accLength == 0 ? v_tgt : (int32_t)sqrtf(v_min2);
}

int32_t LinStepAccelerator::updateSpeed(int32_t curPos)
{
    uint32_t delta = std::abs(s_0 - curPos);

    // acceleration phase -------------------------------------
    if (delta < accLength)
        return sqrtf(two_a * delta + v_min2);

    // constant speed phase ------------------------------------
    if (delta < decStart)
        return v_tgt;

    //deceleration phase --------------------------------------
    return sqrtf(two_a * ((delta < delta_tgt - 1) ? delta_tgt - delta - 2 : 0) + v_min2);
}

uint32_t LinStepAccelerator::initiateStopping(int32_t curPos)
{
    uint32_t stepsDone = std::abs(s_0 - curPos);
    uint32_t stepsToDo;

    if (stepsDone < accLength)    // still accelerating
        stepsToDo = stepsDone; // will take the same distance to stop as we already traveled
    else                       // constant speed phase
        stepsToDo = accLength;

    delta_tgt = stepsDone + stepsToDo;
    accLength = decStart = 0;     // start decelerating now

    return stepsToDo;    
}