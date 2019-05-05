#pragma once

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
    uint32_t stepsDone = std::abs(s_0 - curPos);

    // acceleration phase -------------------------------------
    if (stepsDone < accLength)
        return sqrtf(two_a * stepsDone + v_min2);

    // constant speed phase ------------------------------------
    if (stepsDone < decStart)
        return v_tgt;

    //deceleration phase --------------------------------------
    if(stepsDone < delta_tgt)
        return sqrtf(two_a * ((stepsDone < delta_tgt - 1) ? delta_tgt - stepsDone - 2 : 0) + v_min2);

    //we are done, make sure to return 0 to stop the step timer
    return 0; 
}

uint32_t LinStepAccelerator::initiateStopping(int32_t curPos)
{
    uint32_t stepsDone = std::abs(s_0 - curPos);
    
    if (stepsDone < accLength)              // still accelerating
    {        
        accLength = decStart = 0;           // start deceleration 
        delta_tgt = 2 * stepsDone;          // we need the same way to decelerate as we traveled so far
        return stepsDone;                   // return steps to go
    }
    else if (stepsDone < decStart)          // constant speed phase
    {
        decStart = 0;                       // start deceleration
        delta_tgt = stepsDone + accLength;  // normal deceleration distance 
        return accLength;                   // return steps to go
    }
    else                                    // already decelerating
    {
        return delta_tgt - stepsDone;       // return steps to go
    }
}