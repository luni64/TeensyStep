#include <math.h>
#include "StepControlBase.h"

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class StepControl : public StepControlBase<pulseWidth,accUpdatePeriod>
{
    public:
    StepControl();

  protected:
    uint32_t accelerationEnd;
    uint32_t decelerationStart;
    uint32_t sqrt_2a;
    uint32_t s_tgt, v_tgt, v_min;

    inline uint32_t prepareMovement(uint32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t acceleration);
    inline uint32_t updateSpeed(uint32_t currentPosition);
    inline uint32_t initiateStopping(uint32_t currentPosition);
};


// Implementation =====================================================================================================

template <unsigned p, unsigned u>
StepControl<p, u>::StepControl() : StepControlBase<p,u>()
{      
}


template <unsigned p, unsigned u>
uint32_t StepControl<p,u>::prepareMovement(uint32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t a)
{
    s_tgt = targetPos;
    v_tgt = targetSpeed;
    v_min = pullInSpeed;

    if (v_tgt > v_min)                   // target speed larger than pull in speed
    {                                    // -> acceleration required
        float dv = v_tgt - v_min;
        float ae = dv * dv / (2.0f * a); // length of acceleration phase (steps)
        sqrt_2a = sqrtf(2.0f * a);       // precalc, needed during acceleration

        accelerationEnd = std::min((uint32_t)ae, s_tgt / 2); // limit acceleration phase to half of total steps
        decelerationStart = s_tgt - accelerationEnd;
        return v_min;                    // start movment with the pull in speed
    }
    else                                 // target speed smaller that pull in speed
    {                                    // -> no acceleration necessary
        accelerationEnd = 0;
        decelerationStart = s_tgt;
        return v_tgt;
    }
}

template <unsigned p, unsigned u>
uint32_t StepControl<p,u>::updateSpeed(uint32_t currentPosition)
{
    // acceleration phase -------------------------------------
    if (currentPosition < accelerationEnd)
        return sqrt_2a * sqrtf(currentPosition) + v_min;

    // constant speed phase ------------------------------------
    if (currentPosition < decelerationStart)
        return v_tgt;

    //deceleration phase --------------------------------------
    return sqrt_2a * sqrtf(s_tgt - currentPosition) + v_min;
}

template <unsigned p, unsigned u>
uint32_t StepControl<p,u>::initiateStopping(uint32_t s_cur)
{
    if (s_cur <= decelerationStart)  // we are already decelerationg, nothing to change...
    {
        return s_tgt;
    }
    else                             // accelerating or constant speed phase
    {
        uint32_t newTarget;
        if (s_cur < accelerationEnd) // still accelerating
            newTarget = s_cur * 2;   // will take the same distance to stop as we already traveled
        else                         // constant speed phase
            newTarget = s_cur + accelerationEnd;

        accelerationEnd = 0;
        decelerationStart = s_cur; // start decelerating now
        return newTarget;
    }
}
