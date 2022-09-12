
#include "LinRotAccelerator.h"


typedef struct {
    float v_tgt;
    float v_cur;
    float v_tgt_orig;
    float dv_orig;
    float dv_cur;
    float dv;
}LinRotAcceleratorParam;



void RotAccelerator_prepareRotation(LinRotAccelerator *_accelerator, int32_t currentPosition, int32_t targetSpeed, uint32_t a, uint32_t accUpdatePeriod, float speedFactor)
{
    LinRotAcceleratorParam *accelerator = (LinRotAcceleratorParam *)_accelerator->param;

    accelerator->v_tgt_orig = targetSpeed;
    accelerator->dv_orig = ((float)a * accUpdatePeriod) / 1E6;
    accelerator->v_cur = 0;

    RotAccelerator_overrideAcceleration(_accelerator, 1.0f);
    RotAccelerator_overrideSpeed(_accelerator, speedFactor);
}

void RotAccelerator_overrideSpeed(LinRotAccelerator *_accelerator, float factor)
{
    //Serial.printf("a:------ %d\n", a);
    LinRotAcceleratorParam *accelerator = (LinRotAcceleratorParam *)_accelerator->param;

    noInterrupts();
    accelerator->v_tgt = accelerator->v_tgt_orig * factor;
    accelerator->dv = accelerator->v_tgt > accelerator->v_cur ? accelerator->dv_cur : (-1 * accelerator->dv_cur);
    interrupts();
}


void RotAccelerator_overrideAcceleration(LinRotAccelerator *_accelerator, float factor)
{
    //Serial.printf("a:------ %d\n", a);
    LinRotAcceleratorParam *accelerator = (LinRotAcceleratorParam *)_accelerator->param;
    if (factor > 0)
    {
        noInterrupts();
        accelerator->dv_cur = accelerator->dv_orig * factor;
        accelerator->dv *= factor;
        interrupts();
    }
}

int32_t RotAccelerator_updateSpeed(LinRotAccelerator *_accelerator, int32_t curPos)
{
    (void)curPos;
    LinRotAcceleratorParam *accelerator = (LinRotAcceleratorParam *)_accelerator->param;
    // if((int32_t)accelerator->v_cur == (int32_t)accelerator->v_tgt)
    if(fabsf(accelerator->v_cur - accelerator->v_tgt) < 0.001f)
        return (int32_t)accelerator->v_tgt; // already at target, keep spinning with target frequency

    accelerator->v_cur += accelerator->dv;
    accelerator->v_cur = accelerator->dv > 0.0f ? min(accelerator->v_tgt, accelerator->v_cur) : max(accelerator->v_tgt, accelerator->v_cur);

    return (int32_t)accelerator->v_cur;
}

int32_t RotAccelerator_initiateStopping(LinRotAccelerator *_accelerator, int32_t curPos)
{   
    (void)curPos;
    RotAccelerator_overrideSpeed(_accelerator, 0);
    return 0;
}

