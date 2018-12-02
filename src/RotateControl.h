#include <math.h>
#include <limits>
#include "RotateControlBase.h"

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControl : public RotateControlBase<pulseWidth, accUpdatePeriod>
{
  public:
    RotateControl();
    RotateControl(const RotateControl &) = delete;
    RotateControl &operator=(const RotateControl &) = delete;

  protected:
    uint32_t accelerationStart, accelerationEnd;
    // uint32_t decelerationStart;
    uint32_t sqrt_2a;
    uint32_t v_tgt, v_min, v_cur;

    inline uint32_t prepareRotation(uint32_t currentPosition, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t acceleration);
    inline uint32_t updateSpeed(uint32_t currentPosition);
    inline uint32_t initiateStopping(uint32_t currentPosition);
};

// Implementation =====================================================================================================

template <unsigned p, unsigned u>
RotateControl<p, u>::RotateControl() : RotateControlBase<p, u>()
{
    v_cur = 0;
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::prepareRotation(uint32_t currentPosition, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t a)
{
    v_tgt = targetSpeed;
    v_min = pullInSpeed;


    // target speed smaller than pull in speed-> no acceleration necessary
    if (v_tgt <= v_min) 
    {        
        accelerationEnd = 0;
        v_cur = v_tgt;
        return v_tgt;
    }

    if(v_cur < v_min) v_cur = v_min;

    if (v_tgt > v_cur) 
    {
        sqrt_2a = sqrtf(2.0f * a);       // precalc, needed during acceleration
        uint32_t dv = v_tgt - v_cur;
        uint64_t ae = dv * dv / (2.0f * a); // length of acceleration phase (steps)

        accelerationStart = currentPosition;
        accelerationEnd = accelerationStart + (uint32_t)ae;  ///HACK, insert error handling
        
        Serial.println(accelerationStart);
        Serial.println(accelerationEnd);
        Serial.println();

        return v_cur; // start movment with the pull in speed
    }

    return 0; 
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::updateSpeed(uint32_t currentPosition)
{

    // constant speed phase ------------------------------------
    if (currentPosition > accelerationEnd)
    {
        Serial.println(v_tgt);
        return v_tgt;
    }

    Serial.printf("ret: %d pos:%d\n", sqrt_2a * sqrtf(currentPosition - accelerationStart) + v_min, currentPosition-accelerationStart);

    // acceleration phase -------------------------------------
    return sqrt_2a * sqrtf(currentPosition-accelerationStart) + v_min;

    //if (currentPosition < decelerationStart)

    //deceleration phase --------------------------------------
    // return sqrt_2a * sqrtf(s_tgt - currentPosition) + v_min;
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::initiateStopping(uint32_t s_cur)
{
    // if (s_cur <= decelerationStart) // we are already decelerationg, nothing to change...
    // {
    //     return s_tgt;
    // }
    //else // accelerating or constant speed phase
    //{
    uint32_t newTarget;
    if (s_cur < accelerationEnd) // still accelerating
        newTarget = s_cur * 2;   // will take the same distance to stop as we already traveled
    else                         // constant speed phase
        newTarget = s_cur + accelerationEnd;

    accelerationEnd = 0;
    //decelerationStart = s_cur; // start decelerating now
    return newTarget;
    //}
}
