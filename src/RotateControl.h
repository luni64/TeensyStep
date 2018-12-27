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

    void overrideSpeed(float fac)
    {
        noInterrupts();

        v_tgt = std::round(v_tgt_orig * fac);
        vstp_tgt = ((float)v_tgt * v_tgt) / two_a * (v_tgt > 0 ? 1.0f : -1.0f);

        s_0 = this->leadMotor->getPosition();


        dir = vstp_tgt > vstp ? 1 : -1;

        //Serial.printf("os: dir: %i vstp_tgt: %i vstp:%i\n", dir, vstp_tgt,vstp);
        interrupts()
    }

  protected:
    int32_t a, two_a;
    int32_t dir;

    int32_t v = 0;
    int32_t vstp = 0;

    int32_t v_tgt, v_tgt_orig;
    int32_t vstp_tgt;

    int32_t v_min, v_min_sqr;

    int32_t s_0;

    int32_t prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration);
    int32_t updateSpeed(int32_t currentPosition);
    uint32_t initiateStopping(int32_t currentPosition);

    void prepareMove(int32_t currentPosition, int32_t targetPosition, int32_t targetSpeed, uint32_t acceleration);
};

// Implementation =====================================================================================================

float signed_sqrt(int32_t x) // signed square
{
    return x > 0 ? sqrtf(x) : -sqrtf(-x);
}

template <unsigned p, unsigned u>
int32_t RotateControl<p, u>::prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration)
{
    v_tgt_orig = targetSpeed;
    a = acceleration;
    two_a = 2 * a;
    v_min_sqr = a;
    v_min = sqrtf(v_min_sqr);
    vstp = 0;
    overrideSpeed(1.0f);

    //Serial.printf("%vtgt:%i vstp_tgt:%i  \n", v_tgt, vstp_tgt);
    return v_min;
}

template <unsigned p, unsigned u>
int32_t RotateControl<p, u>::updateSpeed(int32_t currentPosition)
{
    if (vstp == vstp_tgt) // already at target, keep spinning with target frequency
    {      
        return v_tgt;
    }
    vstp += std::abs(currentPosition - s_0) * dir;
    vstp = dir == 1 ?  std::min(vstp_tgt, vstp) : std::max(vstp_tgt, vstp);  // clamp vstp to target

    //Serial.printf("dir: %i, vstp_tgt:%i, vstp:%i, deltaS:%i\n", dir, vstp_tgt, vstp, deltaS);
    s_0 = currentPosition;
    return signed_sqrt(two_a * (vstp-1) + v_min_sqr);    
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::initiateStopping(int32_t s_cur)
{ 
    overrideSpeed(0);
    return 0;
}
