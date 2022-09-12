#ifndef _LINSTEPACCELERATOR_H__
#define _LINSTEPACCELERATOR_H__

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
// #include <algorithm>

#include "ErrorHandler.h"

typedef uint8_t AcceleratorParamBase;

typedef struct {
    // int32_t s_0, ds;
    // uint32_t vs, ve, vt;
    // int64_t vs_sqr, ve_sqr, vt_sqr;
    // uint32_t two_a;
    // int32_t accEnd, decStart;
    AcceleratorParamBase param[64];   ///  space allocate
    errCallback_t errorCallback;
}LinStepAccelerator;


int32_t Accelerator_prepareMovement(LinStepAccelerator *_accelerator, int32_t currentPos, int32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t pullOutSpeed, uint32_t a);

int32_t Accelerator_updateSpeed(LinStepAccelerator *_accelerator, int32_t currentPosition);

uint32_t Accelerator_initiateStopping(LinStepAccelerator *_accelerator, int32_t currentPosition);

void Accelerator_overrideSpeed(LinStepAccelerator *_accelerator, float fac, int32_t currentPosition);






 
#endif
