
#include "SinRotAccelerator.h"

typedef struct {
    int32_t dir;
    int32_t a;
    int32_t two_a;
    int32_t vstp;
    int32_t v_tgt;
    int32_t v_tgt_orig;
    int32_t vstp_tgt;
    int32_t v_min;
    int32_t v_min_sqr;
    int32_t s_0;
}SinRotAcceleratorParam;


static inline float signed_sqrt(int32_t x) // signed square root
{
    return x > 0 ? sqrt(x) : -sqrt(-x);
}

// Implementation =====================================================================================================

int32_t SinRotAccelerator_prepareRotation(SinRotAccelerator *_accelerator, int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration, float speedFactor)
{
    SinRotAcceleratorParam *accelerator = (SinRotAcceleratorParam *)_accelerator->param;

    accelerator->v_tgt_orig = targetSpeed;
    accelerator->a = acceleration;
    accelerator->two_a = 2 * accelerator->a;
    accelerator->v_min_sqr = accelerator->a;
    accelerator->v_min = sqrtf(accelerator->v_min_sqr);
    accelerator->vstp = 0;
    SinRotAccelerator_overrideSpeed(_accelerator, speedFactor, currentPosition);

    //Serial.printf("%vtgt:%i vstp_tgt:%i  \n", v_tgt, vstp_tgt);
    return accelerator->v_min;
}

int32_t SinRotAccelerator_updateSpeed(SinRotAccelerator *_accelerator, int32_t curPos)
{
    if (accelerator->vstp == accelerator->vstp_tgt) // already at target, keep spinning with target frequency
    {
        return accelerator->v_tgt;
    }

    accelerator->vstp += labs(curPos - accelerator->s_0) * accelerator->dir;
    accelerator->vstp = accelerator->dir == 1 ? min(accelerator->vstp_tgt, accelerator->vstp) : max(accelerator->vstp_tgt, accelerator->vstp); // clamp vstp to target

    //Serial.printf("dir: %i, vstp_tgt:%i, vstp:%i, deltaS:%i\n", dir, vstp_tgt, vstp, deltaS);
    accelerator->s_0 = curPos;
    return signed_sqrt(accelerator->two_a * accelerator->vstp + accelerator->v_min_sqr);
}

void SinRotAccelerator_overrideSpeed(SinRotAccelerator *_accelerator, float fac, int32_t curPos)
{
    noInterrupts();
    accelerator->s_0 = curPos;
    accelerator->v_tgt = lroundf(accelerator->v_tgt_orig * fac);
    accelerator->vstp_tgt = ((float)accelerator->v_tgt * accelerator->v_tgt) / accelerator->two_a * (accelerator->v_tgt > 0 ? 1.0f : -1.0f);
    accelerator->dir = accelerator->vstp_tgt > accelerator->vstp ? 1 : -1;
    interrupts();
}

int32_t SinRotAccelerator_initiateStopping(SinRotAccelerator *_accelerator, int32_t curPos)
{
    SinRotAccelerator_overrideSpeed(_accelerator, 0.0f, curPos);
    return 0;
}