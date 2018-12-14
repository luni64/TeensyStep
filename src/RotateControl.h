#pragma once

#include "RotateControlBase.h"
#include <math.h>
#include <limits>

uint32_t SquareRootRounded(uint64_t a_nInput)
{
    uint64_t op = a_nInput;
    uint64_t res = 0;
    uint64_t one = 1uLL << 62; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type

    // "one" starts at the highest power of four <= than the argument.
    while (one > op)
    {
        one >>= 2;
    }

    while (one != 0)
    {
        if (op >= res + one)
        {
            op = op - (res + one);
            res = res + 2 * one;
        }
        res >>= 1;
        one >>= 2;
    }

    return res;
}

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControl : public RotateControlBase<pulseWidth, accUpdatePeriod>
{
  public:
    RotateControl() = default;
    RotateControl(const RotateControl &) = delete;
    RotateControl &operator=(const RotateControl &) = delete;

    void overrideSpeed(int32_t v)
    {
        cli();
        //    Serial.printf("os: %g --------------------------\n", fac);

        v_tgt = v;

        up = v_tgt > v ? 1 : -1;
        two_a = 2 * acc * up;

        if (v_tgt > 0)
        {
            st2 = v_tgt * v_tgt;
        }
        else
        {
            st2 = -(v_tgt * v_tgt);
        }

        sei();
    }


    void overrideSpeed(float fac)
    {
        cli();
        //    Serial.printf("os: %g --------------------------\n", fac);

        v_tgt = v_tgt_orig * fac;

        up = v_tgt > v ? 1 : -1;
        two_a = 2 * acc * up;

        if (v_tgt > 0)
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
    // uint32_t sqrt_2a;
    // int32_t v02_2a;

    // uint32_t decelerationStart;
    int32_t two_a;
    int64_t v2 = 0;
    int32_t v = 0;
    int64_t st2;
    int32_t v_tgt;

    int up;

    int32_t v_tgt_orig, v_min;
    int32_t acc;
    //  float fa;

    //int32_t v2_tgt, v_min, v_cur;
    int32_t s_0;
    int32_t s_tgt;
    int32_t decStart;

    void prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration);
    int32_t updateSpeed(int32_t currentPosition);
    uint32_t initiateStopping(int32_t currentPosition);

    void prepareMove(int32_t currentPosition, int32_t targetPosition, int32_t targetSpeed, uint32_t acceleration);
};

// Implementation =====================================================================================================

template <unsigned p, unsigned u>
void RotateControl<p, u>::prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t a)
{
    v_tgt_orig = targetSpeed;
    acc = a;
    

    s_0 = currentPosition;
    overrideSpeed(1.0f);

    // Serial.printf("Prepare cp:%d, ts:%d, a:%d v:%d two_a:%" PRIi64 "\n", currentPosition, targetSpeed, a, v, two_a);
    // Serial.flush();
}

template <unsigned p, unsigned u>
void RotateControl<p, u>::prepareMove(int32_t currentPosition, int32_t targetPosition, int32_t targetSpeed, uint32_t a)  
{
    prepareRotation(currentPosition, targetSpeed, a);
    s_tgt = targetPosition;
    

   // float dv = std::abs(targetSpeed);// - sqrtf(400);
    float ae = ((float)targetSpeed * targetSpeed - sqrtf(two_a))/ two_a; // length of acceleration phase (steps)

   

    decStart =  s_tgt - (int32_t) ae;

    Serial.printf("Prepare move cp:%d, tp:%d, ae:%f\n", currentPosition, targetPosition, ae);

}

template <unsigned p, unsigned u>
int32_t RotateControl<p, u>::updateSpeed(int32_t currentPosition)
{
   // Serial.printf("Update cp:%i s_tgt:%i v_tgt:%i \n", currentPosition, s_tgt, v_tgt);

    if (currentPosition >= decStart && v_tgt != (int32_t)sqrtf(two_a))
    {      
        digitalWriteFast(15, HIGH);
        overrideSpeed((int32_t)sqrt(two_a));
        delayMicroseconds(1);
        digitalWriteFast(15, LOW);
    }

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

    v2 += (two_a * delta_s);
    v2 = (up == 1) ? std::min(v2, st2) : std::max(v2, st2);

    // v = v2 > 0 ? sqrtf(v2) : -sqrtf(-v2);
    v = v2 > 0 ? SquareRootRounded(v2) : -SquareRootRounded(-v2);
    //Serial.printf("std, v:%i \n", v);
    return v;
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::initiateStopping(int32_t s_cur)
{

    //Serial.printf("stp %d\n", s_cur);
    overrideSpeed(0.0f);
    return 0;
}
