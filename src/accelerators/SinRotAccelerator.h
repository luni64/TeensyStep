#ifndef _SINROTACCELERATOR_H__
#define _SINROTACCELERATOR_H__

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "../ErrorHandler.h"

#include "../port/port.h"

typedef uint8_t SinRotAcceleratorParamBase;

typedef struct {
    // int32_t dir;
    // int32_t a, two_a; 
    // int32_t vstp;
    // int32_t v_tgt, v_tgt_orig, vstp_tgt;
    // int32_t v_min, v_min_sqr;
    // int32_t s_0;
    SinRotAcceleratorParamBase param[40];
    errCallback_t errorCallback;
}SinRotAccelerator;


int32_t SinRotAccelerator_prepareRotation(SinRotAccelerator *_accelerator, int32_t currentPosition, int32_t targetSpeed, uint32_t acceleration, float speedFactor = 1.0);

int32_t FUN_IN_RAM SinRotAccelerator_updateSpeed(SinRotAccelerator *_accelerator, int32_t currentPosition);

int32_t SinRotAccelerator_initiateStopping(SinRotAccelerator *_accelerator, int32_t currentPosition);

void SinRotAccelerator_overrideSpeed(SinRotAccelerator *_accelerator, float fac, int32_t currentPosition);


#endif
