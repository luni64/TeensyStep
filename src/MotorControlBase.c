
#include "MotorControlBase.h"

static ErrFunc errFunc = NULL;

MotorControlBase* Controller_init(MotorControlBase* controller, const MotorControlBase_Init_TypeDef *config){

    TimerField_setPulseWidth(&controller->timerField, config->pulseWidth);
    TimerField_setAccUpdatePeriod(&controller->timerField, config->accUpdatePeriod);
    
    controller->reachedTargetCallback = config->reachedTargetCallback;
    controller->accUpdatePeriod = config->accUpdatePeriod;
    controller->pulseWidth = config->pulseWidth;
    controller->mCnt = 0;

    return controller;
}

int32_t Controller_getCurrentSpeed(const MotorControlBase *controller){
    return TimerField_getStepFrequency(&controller->timerField);
}


void Controller_attachStepper(MotorControlBase *controller, uint8_t N, Stepper* *steppers){

    ASSERT(N <= MAXMOTORS);

    for(size_t i = 0; i < N; i++){
        controller->motorList[i] = steppers[i];
    }
    controller->motorList[N] = NULL;
}

void vController_attachStepper(MotorControlBase *controller, uint8_t N, ...){
    ASSERT(N <= MAXMOTORS);

    va_list mlist;

    va_start(mlist, N);

    for(size_t i = 0; i < N; i++){
        controller->motorList[i] = (Stepper *)va_arg(mlist, Stepper*);
    }
    va_end(mlist);

    controller->motorList[N] = NULL;
}

void vvController_attachStepper(MotorControlBase *controller, uint8_t N, __builtin_va_list va){
    ASSERT(N <= MAXMOTORS);

    for(size_t i = 0; i < N; i++){
        controller->motorList[i] = (Stepper *)va_arg(va, Stepper*);
    }
    controller->motorList[N] = NULL;
}

void attachErrorFunction(ErrFunc ef){
    errFunc = ef;
}

void Error(ErrCode e){
    if (errFunc != NULL) errFunc(e);
}
