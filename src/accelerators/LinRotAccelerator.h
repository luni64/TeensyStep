#pragma once

#include "wiring.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

class LinRotAccelerator
{
  public:
    inline void prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration, uint32_t accUpdatePeriod, float speedFactor = 1.0);
    inline int32_t updateSpeed(int32_t currentPosition);
    inline int32_t initiateStopping(int32_t currentPosition);
    inline void overrideSpeed(float fac, int32_t currentPosition);

    LinRotAccelerator() = default;

  protected:
    LinRotAccelerator(const LinRotAccelerator &) = delete;
    LinRotAccelerator &operator=(const LinRotAccelerator &) = delete;

    int32_t v_tgt, v_cur;
    int32_t v_tgt_orig, dv_orig, dv;
};

// Inline Implementation =====================================================================================================

void LinRotAccelerator::prepareRotation(int32_t currentPosition, int32_t targetSpeed, uint32_t a, uint32_t accUpdatePeriod, float speedFactor)
{
    v_tgt_orig = targetSpeed;
    dv_orig = (a / 1000) * (accUpdatePeriod / 1000);
    v_cur = 325;

    overrideSpeed(speedFactor, currentPosition);
}

int32_t LinRotAccelerator::updateSpeed(int32_t curPos)
{
    if (v_cur == v_tgt) return v_tgt; // already at target, keep spinning with target frequency

    v_cur += dv;
    v_cur = dv > 0 ? std::min(v_tgt, v_cur) : std::max(v_tgt, v_cur);

    return v_cur;
}

void LinRotAccelerator::overrideSpeed(float fac, int32_t curPos)
{
    noInterrupts();
    v_tgt = v_tgt_orig * fac;
    dv = v_tgt > v_cur ? dv_orig : -dv_orig;
    interrupts();
}

int32_t LinRotAccelerator::initiateStopping(int32_t curPos)
{
    overrideSpeed(0, curPos);
    return 0;
}