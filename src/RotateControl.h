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

    void overrideSpeed(int32_t vv)
    {
        cli();

        v_tgt = vv;

        int up = v_tgt > this->leadMotor->getSpeed() ? 1 : -1;
        two_a = 2 * acc * up;

        st2 = (v_tgt > 0) ? v_tgt * v_tgt : -(v_tgt * v_tgt);


        sei();
    }

    void overrideSpeed(float fac)
    {
        cli();
        //    Serial.printf("os: %g --------------------------\n", fac);

        v_tgt = v_tgt_orig * fac;

        int up = v_tgt > this->leadMotor->getSpeed() ? 1 : -1;
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
   // int32_t v = 0;
    int64_t st2;
    int32_t v_tgt;

    // int up;

    int32_t v_tgt_orig, v_min;
    int32_t acc;
    //  float fa;

    int32_t s_0;
    int32_t s_tgt;
    int32_t decStart;

    bool decelerating;

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
    acc = a;
    v_tgt_orig = targetSpeed;
    s_tgt = targetPosition;
    s_0 = currentPosition;
    v_min = sqrtf(2.0 * a);
    decelerating = false;

    int32_t accLength = ((float)targetSpeed * targetSpeed) / (2.0f * a) - 1.0f; // s = (vt^2 - v0^2)/2a; v0^2 = 2a

    decStart = s_tgt - this->leadMotor->dir * accLength;
    decelerating = false;

    Serial.printf("Prepare move cp:%d, tp:%d, ts:%d, al:%i, ds:%i\n", currentPosition, targetPosition, targetSpeed, accLength, decStart);
    overrideSpeed(1.0f);
}

template <unsigned p, unsigned u>
int32_t RotateControl<p, u>::updateSpeed(int32_t currentPosition)
{
    int dd = (currentPosition - decStart) * this->leadMotor->dir;
    //Serial.printf("Update cp:%i, ds: %i, dir: %i, s_tgt:%i dd:%i \n", currentPosition, decStart, this->leadMotor->dir, s_tgt, dd);
    if (decelerating == false && dd >= 0)
    {
        digitalWriteFast(15, HIGH);
        decelerating = true;
        s_0 = decStart;
        Serial.printf("dec %i ------------------------\n", this->leadMotor->dir * (int32_t)sqrtf(2 * acc));
        overrideSpeed(this->leadMotor->dir * v_min);
        digitalWriteFast(15, LOW);
        //return v;
    }

    if (this->leadMotor->getSpeed()== v_tgt)
    {
        Serial.println("v=vtgt");
        s_0 = currentPosition;
        return v_tgt;
    }
    if (this->leadMotor->getSpeed() == 0) // Last speed was 0, target != zero -> start movment
    {
        Serial.println("v=0");
        s_0 = currentPosition;
        v2 = two_a;

        //v2 = two_a > 0 ? std::min(st2, v2) : std::max(st2, v2);
        //v2 = (up == 1) ? std::min(v2, st2) : std::max(v2, st2);
        return v2 > 0 ? sqrtf(v2) : -sqrtf(-v2);
        //Serial.printf("zeor, v:%i st2 %f twoa: %i \n", v,st2,two_a);
        //return v;
    }

    digitalWriteFast(16, HIGH);
    int32_t delta_s = std::abs(currentPosition - s_0);
    s_0 = currentPosition;
    v2 += (two_a * delta_s);
    v2 = two_a > 0 ? std::min(st2, v2) : std::max(st2, v2);
    int32_t v = v2 > 0 ? SquareRootRounded(v2) : -SquareRootRounded(-v2);
   // Serial.printf("std, v:%i ds:%i, st2:%g v2:%g \n", v, delta_s, (float)st2, (float)v2);
    digitalWriteFast(16, LOW);
    return v;
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::initiateStopping(int32_t s_cur)
{

    //Serial.printf("stp %d\n", s_cur);
    overrideSpeed(0.0f);
    return 0;
}
