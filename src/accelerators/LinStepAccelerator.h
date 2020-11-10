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
    vs = std::min(pullInSpeed, vt);  // v_start
    ve = std::min(pullOutSpeed, vt); // v_end
    two_a = 2 * a;

    s_0 = currentPos;
    ds = std::abs(targetPos - currentPos);

    vs_sqr = vs * vs;
    ve_sqr = ve * ve;
    vt_sqr = vt * vt;

    int32_t sa = (vt_sqr - vs_sqr) / two_a; // required distance to reach target speed, starting with start speed
    int32_t se = (vt_sqr - ve_sqr) / two_a; // required distance to reach end speed, starting with target speed

    //Serial.printf("ve: %d, vs: %d, vt: %d, ds: %d, sa: %d, se: %d\r\n", ve, vs, vt, ds, sa, se);

    if(sa + se > ds)
    {
        // target speed cannot be reached, need to calculate new max speed with sa + se = ds
        vt_sqr = (two_a*ds + ve_sqr + vs_sqr) / 2; // new speed that fullfills sa + se = ds
        sa = (vt_sqr - vs_sqr) / two_a; // acc distance
        se = (vt_sqr - ve_sqr) / two_a; // dec distance

        // adjust for cases where sa + se = ds is fullfilled but either sa or se is negative. A
        // negative value for sa means the deceleration phase is too short and the only solution
        // to reach the desired end speed is to decelerate more steps than ds allows. As we cannot run more steps
        // we must truncate these and allow a higher end speed than ve.
        // A negative value for se means the acceleartion phase is too short and we must allow a lower end
        // speed than ve.
        if(sa < 0)
        {
            se += sa;
            sa = 0;
        }
        if(se < 0)
        {
            sa += se;
            se = 0;
        }

        if(sa + se != ds){
            // catch truncation in above calculations. The acc/dec phase may differ with one due to
            // truncation so we catch it here and add it to either acc or dec.
            // If we don't catch it the updateSpeed function will enter constant speed and return the
            // target speed no matter what speed the acc phase ended at and could case a way too
            // high acceleration for a short period before entering the dec phase.
            sa != 0 ? sa++ : se++;
        }
        //Serial.printf("recalculating, new v: %d, sa: %d, se: %d\r\n",(int32_t)sqrtf(vt_sqr) ,sa, se);
    }
    accEnd = sa;
    decStart = ds - se;
    //Serial.printf("acc end: %d, dec start: %d, total: %d\r\n", accEnd, decStart, ds);
    return vs;
}

int32_t LinStepAccelerator::updateSpeed(int32_t curPos)
{

    int32_t s = std::abs(s_0 - curPos);

    // acceleration phase -------------------------------------
    if (s < accEnd)
    {
        int32_t res = sqrtf(two_a * s + vs_sqr);
        //Serial.printf("start speed: %d, s: %d, accEnd: %d\r\n", res, s, accEnd);
        return res;
    }

    // constant speed phase ------------------------------------
    if (s < decStart)
    {
        //Serial.printf("constant vt: %d, s: %d\r\n", vt, s);
        return vt;
    }

    //deceleration phase --------------------------------------
    if (s < ds)
    {
        //  return sqrtf(two_a * ((stepsDone < ds - 1) ? ds - stepsDone - 2 : 0) + vs_sqr);
        int32_t res = sqrtf(ve_sqr + (ds - s - 1) * two_a);
        //Serial.printf("End speed: %d, s: %d, decStart: %d\r\n", res, s, decStart);
        return res;
    }

    //Serial.printf("Total steps: %d\r\n", s);
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
