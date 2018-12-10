#pragma once

#include "RotateControlBase.h"
#include <math.h>
#include <limits>

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControl : public RotateControlBase<pulseWidth, accUpdatePeriod>
{
  public:
    RotateControl() = default;
    RotateControl(const RotateControl &) = delete;
    RotateControl &operator=(const RotateControl &) = delete;

    void overrideSpeed(float fac)
    {
        cli();
    //    Serial.printf("os: %g --------------------------\n", fac);

        v_tgt = v_tgt_orig * fac;

        up = v_tgt > v ? 1 : -1;
        two_a = 2 * acc * up;        
     
        if(v_tgt > 0)
        {
            st2 = v_tgt * v_tgt;
        }
        else
        {
            st2 = -(v_tgt * v_tgt);
        }
        sei();
    }

  protected:
    // uint32_t decelerationStart;
    int64_t two_a;
    int64_t v2 = 0;
    int32_t v = 0;
    int64_t st2;
    int32_t v_tgt;

    int up;

    int32_t v_tgt_orig;
    int32_t acc;

    //int32_t v2_tgt, v_min, v_cur;
    int32_t s_0;
    //bool sstop = false;

    void prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration);
    int32_t updateSpeed(int32_t currentPosition);
    uint32_t initiateStopping(uint32_t currentPosition);   
};

// Implementation =====================================================================================================


template <unsigned p, unsigned u>
void RotateControl<p, u>::prepareRotation(int32_t currentPosition, int32_t targetSpeed,  uint32_t a)
{
    v_tgt_orig = targetSpeed;
    acc = a;
    s_0 = currentPosition;
    overrideSpeed(1.0f);
  
    Serial.printf("Prepare cp:%d, ts:%d, a:%d v:%d two_a:%" PRIi64 "\n", currentPosition, targetSpeed,  a, v, two_a);
    Serial.flush();   
}

template <unsigned p, unsigned u>
int32_t RotateControl<p, u>::updateSpeed(int32_t currentPosition)
{
    //Serial.printf("Update v:%i vtgt:%i up:%d\n",v,v_tgt,up);
    if (v == v_tgt)
    {
        s_0 = currentPosition;
        return v;
    }
    if (v == 0) // Last speed was 0, target != zero -> start movment
    {
        s_0 = currentPosition;
        v2 = two_a;
        v2 = (up == 1) ? std::min(v2, st2) : std::max(v2, st2);
        v = v2 > 0 ? sqrtf(v2) : -sqrtf(-v2);

        //Serial.printf("last zero, ret:%i\n", v);
        return v;
    }

    int32_t delta_s = std::abs(currentPosition - s_0);
    s_0 = currentPosition;

    v2 += two_a * delta_s;
    v2 = (up == 1) ? std::min(v2, st2) : std::max(v2, st2);
   
    v = v2 > 0 ? sqrtf(v2) : -sqrtf(-v2);
    //Serial.printf("std, v:%i \n", v);
    return v;
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::initiateStopping(uint32_t s_cur)
{

    Serial.printf("stp %d\n", s_cur);
    overrideSpeed(0);
    return 0;
}
