#ifndef _LINROTACCELERATOR_H__
#define _LINROTACCELERATOR_H__


#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "../ErrorHandler.h"

#include "../port/port.h"

typedef uint8_t LinRotAcceleratorParamBase;

typedef struct {
    // float v_tgt, v_cur;
    // float v_tgt_orig, dv_orig, dv_cur, dv;
    LinRotAcceleratorParamBase param[24];
    errCallback_t errorCallback;
}LinRotAccelerator;


void RotAccelerator_prepareRotation(LinRotAccelerator *_accelerator, int32_t currentPosition, int32_t targetSpeed, uint32_t a, uint32_t accUpdatePeriod, float speedFactor);


void RotAccelerator_overrideSpeed(LinRotAccelerator *_accelerator, float factor);

void RotAccelerator_overrideAcceleration(LinRotAccelerator *_accelerator, float factor);

int32_t RotAccelerator_updateSpeed(LinRotAccelerator *_accelerator, int32_t curPos);

int32_t RotAccelerator_initiateStopping(LinRotAccelerator *_accelerator, int32_t curPos);


#endif
