#pragma once

#include "RotateControlBase.h"
#include <cmath>
#include <limits>

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControl : public RotateControlBase<pulseWidth, accUpdatePeriod>
{
  public:
    RotateControl() = default;
    RotateControl(const RotateControl &) = delete;
    RotateControl &operator=(const RotateControl &) = delete;

    void overrideSpeed(float fac);
    
  protected:
    int32_t dir;
    int32_t a, two_a;
    int32_t vstp;
    int32_t v_tgt, v_tgt_orig, vstp_tgt;
    int32_t v_min, v_min_sqr;
    int32_t s_0;

    int32_t prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration, float speedFactor = 1.0);
    int32_t updateSpeed(int32_t currentPosition);
    uint32_t initiateStopping(int32_t currentPosition);
};

// Implementation =====================================================================================================

float signed_sqrt(int32_t x) // signed square
{
    return x > 0 ? sqrtf(x) : -sqrtf(-x);
}

template <unsigned p, unsigned u>
int32_t RotateControl<p, u>::prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration, float speedFactor)
{
    v_tgt_orig = targetSpeed;
    a = acceleration;
    two_a = 2 * a;
    v_min_sqr = a;
    v_min = sqrtf(v_min_sqr);
    vstp = 0;
    overrideSpeed(speedFactor);

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
    vstp = dir == 1 ? std::min(vstp_tgt, vstp) : std::max(vstp_tgt, vstp); // clamp vstp to target

    //Serial.printf("dir: %i, vstp_tgt:%i, vstp:%i, deltaS:%i\n", dir, vstp_tgt, vstp, deltaS);
    s_0 = currentPosition;
    return signed_sqrt(two_a * (vstp - 1) + v_min_sqr);
}

template <unsigned p, unsigned u>
void RotateControl<p, u>::overrideSpeed(float fac)
{
    noInterrupts();
    s_0 = this->leadMotor->getPosition();
    v_tgt = std::round(v_tgt_orig * fac);
    vstp_tgt = ((float)v_tgt * v_tgt) / two_a * (v_tgt > 0 ? 1.0f : -1.0f);
    dir = vstp_tgt > vstp ? 1 : -1;
    interrupts()
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::initiateStopping(int32_t s_cur)
{
    overrideSpeed(0);
    return 0;
}
