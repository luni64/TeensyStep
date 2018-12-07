#include "RotateControlBase.h"
#include <math.h>
#include <limits>

// Finds the integer square root of a positive number of any type

// uint32_t intSqrt (uint64_t remainder)
// {
//   uint64_t place = (uint64_t)1 << (sizeof (uint64_t) * 8 - 2); // calculated by precompiler = same runtime as: place = 0x40000000
//   while (place > remainder)
//     place /= 4; // optimized by complier as place >>= 2

//   uint64_t root = 0;
//   while (place)
//   {
//     if (remainder >= root+place)
//     {
//       remainder -= root+place;
//       root += place * 2;
//     }
//     root /= 2;
//     place /= 4;
//   }
//   return root;
// }

template <unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000>
class RotateControl : public RotateControlBase<pulseWidth, accUpdatePeriod>
{
  public:
    RotateControl();
    RotateControl(const RotateControl &) = delete;
    RotateControl &operator=(const RotateControl &) = delete;

    void overrideSpeed(float fac)
    {
        cli();
        v_tgt = v_tgt_orig * fac;
        st2 = (uint64_t)v_tgt * v_tgt;

        Serial.printf("tgt %i\n", v_tgt);
        Serial.flush();

        up = v_tgt > v ? 1 : -1;
        two_a = 2 * (int)acc * up;

        Serial.printf("twoa %" PRIi64 "\n", two_a);
        Serial.flush();

        sei();
    }

  protected:
    // uint32_t decelerationStart;
    int64_t two_a;
    int64_t v2;
    int64_t st2;
    int32_t v = 0;
    int s0;

    int up;

    int s_old;

    int32_t v_tgt_orig;
    int32_t v_tgt;

    int32_t v2_tgt, v_min, v_cur;
    uint32_t s_0;
    bool sstop = false;

    int32_t acc;

    uint32_t prepareRotation(uint32_t currentPosition, int32_t targetSpeed, uint32_t pullInSpeed, uint32_t acceleration);
    int32_t updateSpeed(uint32_t currentPosition);
    uint32_t initiateStopping(uint32_t currentPosition);
    uint32_t updateSpeed2(Stepper *stepper);
};

// Implementation =====================================================================================================

template <unsigned p, unsigned u>
RotateControl<p, u>::RotateControl() : RotateControlBase<p, u>()
{
    v2 = 0;
    s0 = 0;
    st2 = 0;
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::prepareRotation(uint32_t currentPosition, int32_t targetSpeed, uint32_t pullInSpeed, uint32_t a)
{
    s0 = currentPosition;
    st2 = (uint64_t)targetSpeed * targetSpeed;
    v_min = pullInSpeed;
    v_tgt_orig = targetSpeed;
    v_tgt = targetSpeed;

    acc = a;
    up = v_tgt > v ? 1 : -1;
    two_a = 2 * (int)acc * up;

    v = std::max(v, (int32_t)pullInSpeed);
    v2 = (int64_t)v * v;

    Serial.printf("cp:%d, ts:%d, pi:%d, a:%d v:%d two_a%" PRIi64 "\n", currentPosition, targetSpeed, pullInSpeed, a, v, two_a);

    return v;
}

template <unsigned p, unsigned u>
int32_t RotateControl<p, u>::updateSpeed(uint32_t currentPosition)
{
    if (v == v_tgt)
    {
        s0 = currentPosition;
        return v;
    }

    int32_t ds = std::abs((int)currentPosition - s0);
    s0 = currentPosition;

    // Serial.printf("v:%d delta %i", v, delta);

    v2 += two_a * ds;
    v2 = (up == 1) ? std::min(v2, st2) : std::max(v2, st2);

    v = v2 > 0 ? sqrtf(v2) : -sqrtf(-v2);

    // Serial.printf("v:%d tgt:%d\n", v, v_tgt);
    // Serial.flush();

    if (v < v_min)
    {
        v = 0;
    }
    //Serial.printf("curpos:%d v:%d st2:%" PRIu64 "\n", currentPosition, v, st2);
    return v;
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::updateSpeed2(Stepper *stepper)
{
    // if (v == v_tgt)
    // {
    //     s0 = stepper->current;
    //     return v;
    // }

    // int32_t ds = std::abs((int)stepper->current - s0);
    // s0 = stepper->current;

    // // Serial.printf("v:%d delta %i", v, delta);

    // v2 += two_a * ds;
    // v2 = (up == 1) ? std::min(v2, st2) : std::max(v2, st2);

    // v = v2 > 0 ? sqrtf(v2) : -sqrtf(-v2);

    // Serial.printf("v:%d tgt:%d\n", v, v_tgt);
    // Serial.flush();

    // if (v < v_min)
    // {
    //     v = 0;
    // }
    // //Serial.printf("curpos:%d v:%d st2:%" PRIu64 "\n", currentPosition, v, st2);
    return v;
}

template <unsigned p, unsigned u>
uint32_t RotateControl<p, u>::initiateStopping(uint32_t s_cur)
{
    cli()
        Serial.printf("stp %d\n", s_cur);
    prepareRotation(s_cur, 0, 123, 5000);

    sei();
    // sstop = true;
    return 456;
}
