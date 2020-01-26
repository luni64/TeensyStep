#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

class SinSpeedAccelerator
{
  public:
    inline int32_t prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration, float speedFactor = 1.0);
    inline int32_t updateSpeed(int32_t currentPosition);
    inline int32_t initiateStopping(int32_t currentPosition);
    inline void overrideSpeed(float fac, int32_t currentPosition);
    inline bool isMoving();

    SinSpeedAccelerator() = default;

  protected:
    SinSpeedAccelerator(const SinSpeedAccelerator &) = delete;
    SinSpeedAccelerator &operator=(const SinSpeedAccelerator &) = delete;

    int32_t dir;
    int32_t a, two_a; 
    int32_t vstp;
    int32_t v_tgt, v_tgt_orig, vstp_tgt;
    int32_t v_min, v_min_sqr;
    int32_t s_0;

    inline float signed_sqrt(int32_t x) // signed square root
    {
        return x > 0 ? sqrtf(x) : -sqrtf(-x);
    }
};

// Implementation =====================================================================================================

int32_t SinSpeedAccelerator::prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration, float speedFactor)
{
    v_tgt_orig = targetSpeed;
    a = acceleration;
    two_a = 2 * a;
    v_min_sqr = a;
    v_min = sqrtf(v_min_sqr);
    vstp = 0;
    overrideSpeed(speedFactor, currentPosition);

    //Serial.printf("%vtgt:%i vstp_tgt:%i  \n", v_tgt, vstp_tgt);
    return v_min;
}

int32_t SinSpeedAccelerator::updateSpeed(int32_t curPos)
{
    if (vstp == vstp_tgt) // already at target, keep spinning with target frequency
    {
        return v_tgt;
    }

    vstp += std::abs(curPos - s_0) * dir;
    vstp = dir == 1 ? std::min(vstp_tgt, vstp) : std::max(vstp_tgt, vstp); // clamp vstp to target

    //Serial.printf("dir: %i, vstp_tgt:%i, vstp:%i, deltaS:%i\n", dir, vstp_tgt, vstp, deltaS);
    s_0 = curPos;
    return signed_sqrt(two_a * vstp + v_min_sqr);
}

void SinSpeedAccelerator::overrideSpeed(float fac, int32_t curPos)
{
    // TODO: add interrupt handling
    //noInterrupts();
    s_0 = curPos;
    v_tgt = std::round(v_tgt_orig * fac);
    vstp_tgt = ((float)v_tgt * v_tgt) / two_a * (v_tgt > 0 ? 1.0f : -1.0f);
    dir = vstp_tgt > vstp ? 1 : -1;
    //interrupts();
}

int32_t SinSpeedAccelerator::initiateStopping(int32_t curPos)
{
    overrideSpeed(0, curPos);
    return 0;
}

bool SinSpeedAccelerator::isMoving()
{
    return vstp != 0;
}
