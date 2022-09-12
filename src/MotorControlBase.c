
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

int32_t Controller_getCurrentSpeed(MotorControlBase *controller){
    return TimerField_getStepFrequency(&controller->timerField);
}


void Controller_attachStepper(MotorControlBase *controller, uint8_t numbers, Stepper* *steppers){

    ASSERT(numbers <= MAXMOTORS);

    for(size_t i = 0; i < numbers; i++){
        controller->motorList[i] = steppers[i];
    }
    controller->motorList[numbers] = NULL;
}

void vController_attachStepper(MotorControlBase *controller, uint8_t numbers, ...){
    ASSERT(numbers <= MAXMOTORS);

    va_list mlist;

    va_start(mlist, numbers);

    for(size_t i = 0; i < numbers; i++){
        controller->motorList[i] = (Stepper *)va_arg(mlist, Stepper*);
    }
    va_end(mlist);

    controller->motorList[numbers] = NULL;
}

void attachErrorFunction(ErrFunc ef){
    errFunc = ef;
}

void Error(ErrCode e){
    if (errFunc != NULL) errFunc(e);
}
