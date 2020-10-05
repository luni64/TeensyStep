#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

class LinStepAccelerator
{
public:
    inline int32_t prepareMovement(int32_t currentPos, int32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t pullOutSpeed, uint32_t a);
    inline int32_t updateSpeed(int32_t currentPosition);
    inline uint32_t initiateStopping(int32_t currentPosition);
    inline void overrideSpeed(float fac, int32_t currentPosition);

    LinStepAccelerator() = default;

protected:
    LinStepAccelerator(const LinStepAccelerator &) = delete;
    LinStepAccelerator &operator=(const LinStepAccelerator &) = delete;

    int32_t s_0, ds;
    uint32_t vs, ve, vt;
    int64_t vs_sqr, ve_sqr, vt_sqr;
    uint32_t two_a;
    int32_t accEnd, decStart;
};

// Inline Implementation =====================================================================================================

int32_t LinStepAccelerator::prepareMovement(int32_t currentPos, int32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t pullOutSpeed, uint32_t a)
{
    vt = targetSpeed;
    vs = pullInSpeed;  // v_start
    ve = pullOutSpeed; // v_end
    two_a = 2 * a;

    s_0 = currentPos;
    ds = std::abs(targetPos - currentPos);

    vs_sqr = vs * vs;
    ve_sqr = ve * ve;
    vt_sqr = vt * vt;

    int32_t sm = ((ve_sqr - vs_sqr) / two_a + ds) / 2; // position where acc and dec curves meet

    // Serial.printf("ve: %d\n", ve);
    // Serial.printf("vs: %d\n", vs);
    // Serial.printf("ds: %d\n", ds);
    // Serial.printf("sm: %i\n", sm);

    if (sm >= 0 && sm <= ds) // we can directly reach the target with the given values vor v0, ve and a
    {
        int32_t sa = (vt_sqr - vs_sqr) / two_a; // required distance to reach target speed
        if (sa < sm)                              // target speed can be reached
        {
            accEnd = sa;
            decStart = sm + (sm - sa);
            //Serial.printf("reachable accEnd: %i decStart:%i\n", accEnd, decStart);
        }
        else
        {
            accEnd = decStart = sm;
            //Serial.printf("limit accEnd: %i decStart:%i\n", accEnd, decStart);
        }
    }
    else
    {
        // hack, call some error callback instead
        while (1)
        {
            digitalToggle(LED_BUILTIN);
            delay(25);
        }
    }
    return vs;
}

int32_t LinStepAccelerator::updateSpeed(int32_t curPos)
{

    int32_t s = std::abs(s_0 - curPos);

    // acceleration phase -------------------------------------
    if (s < accEnd)
    {
        return sqrtf(two_a * s + vs_sqr);
    }

    // constant speed phase ------------------------------------
    if (s < decStart)
    {
        return vt;
    }

    //deceleration phase --------------------------------------
    if (s < ds)
    {
        //  return sqrtf(two_a * ((stepsDone < ds - 1) ? ds - stepsDone - 2 : 0) + vs_sqr);
        return sqrtf(ve_sqr + (ds - s - 1) * two_a);
    }

    //we are done, make sure to return 0 to stop the step timer
    return 0;
}

uint32_t LinStepAccelerator::initiateStopping(int32_t curPos)
{
    int32_t stepsDone = std::abs(s_0 - curPos);

    if (stepsDone < accEnd) // still accelerating
    {
        accEnd = decStart = 0; // start deceleration
        ds = 2 * stepsDone;    // we need the same way to decelerate as we traveled so far
        return stepsDone;      // return steps to go
    }
    else if (stepsDone < decStart) // constant speed phase
    {
        decStart = 0;            // start deceleration
        ds = stepsDone + accEnd; // normal deceleration distance
        return accEnd;           // return steps to go
    }
    else // already decelerating
    {
        return ds - stepsDone; // return steps to go
    }
}
