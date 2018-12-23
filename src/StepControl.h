#include <cmath>
#include "StepControlBase.h"

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class StepControl : public StepControlBase<pulseWidth, accUpdatePeriod>
{  
    int32_t s_0;
    uint32_t delta_tgt;
    uint32_t accEnd, decStart;
    uint32_t two_a;
    uint32_t v_tgt, v_min2;

    inline uint32_t prepareMovement(int32_t currentPos, int32_t targetPos, uint32_t targetSpeed, uint32_t acceleration);
    inline uint32_t updateSpeed(int32_t currentPos);
    inline uint32_t initiateStopping(int32_t currentPos);
};

template <unsigned p, unsigned u>
uint32_t StepControl<p, u>::prepareMovement(int32_t currentPos, int32_t targetPos, uint32_t targetSpeed, uint32_t a)
{
    s_0 = currentPos;
    delta_tgt = std::abs(targetPos - currentPos);
    v_tgt = targetSpeed;
    two_a = v_min2 = 2 * a;                                 // v^2 - v0^2 = 2as -> v0^2 = 2as  (1)
    uint32_t v_min = sqrtf(v_min2);

    if (v_tgt > v_min)                                      // target speed larger than pull in speed -> acceleration required
    {        
        uint32_t ae = ((float)v_tgt * v_tgt)/two_a - 1.0f;  // length of acceleration phase (we use a float here to avoid overflow in v_tgt^2). Use (1) and vmin^2 = 2a
        accEnd = std::min(ae, delta_tgt / 2);               // limit acceleration phase to half of total steps
        decStart = delta_tgt - accEnd;       
        return v_min;
    }
    else                                                    // target speed smaller than v_min in speed -> no acceleration necessary
    {
        accEnd = 0;
        decStart = delta_tgt;
        return v_tgt;
    }
}

template <unsigned p, unsigned u>
uint32_t StepControl<p, u>::updateSpeed(int32_t currentPos)
{
    uint32_t delta = std::abs(s_0 - currentPos);

    // acceleration phase -------------------------------------
    if (delta < accEnd) return sqrtf(two_a * delta + v_min2);
    
    // constant speed phase ------------------------------------
    if (delta < decStart) return v_tgt;

    //deceleration phase --------------------------------------
    return sqrtf(two_a * ((delta < delta_tgt - 1) ? delta_tgt - delta - 2 : 0) + v_min2);
}

template <unsigned p, unsigned u>
uint32_t StepControl<p, u>::initiateStopping(int32_t currentPos)
{
    uint32_t delta = std::abs(delta_tgt - currentPos);

    if (delta <= decStart) // we are already decelerationg, nothing to change...
    {
        return delta_tgt;
    }
    else // accelerating or constant speed phase
    {
        uint32_t newTarget;
        if (delta < accEnd)           // still accelerating
            newTarget = delta * 2;    // will take the same distance to stop as we already traveled
        else                          // constant speed phase
            newTarget = delta + accEnd;

        accEnd = 0;
        decStart = delta;             // start decelerating now
        return newTarget;
    }
}