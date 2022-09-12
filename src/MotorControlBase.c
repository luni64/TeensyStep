
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

void stepTimerISR(MotorControlBase *controller){
    
    if(Stepper_isClearStepPin(controller->leadMotor) == false) return;
    Stepper_doStep(controller->leadMotor);  /// leadMotor=MotorList[0]

    Stepper* *slave = controller->motorList;
    
    // move slave motors if required (https://en.wikipedia.org/wiki/Bresenham)
    while(*(++slave) != NULL){  // Skip MotorList[0]
        if((*slave)->B >= 0){
            Stepper_doStep(*slave);
            (*slave)->B -= controller->leadMotor->A;
        }
        (*slave)->B += (*slave)->A;
    }

    TimerField_triggerDelay(&controller->timerField);  // start delay line to dactivate all step pins

    // 正向限位
    if((controller->mode == MOTOR_TARGET) && 
        controller->leadMotor->targetPosLimit &&
        (controller->leadMotor->current == controller->leadMotor->targetPosLimit))
    {
        // TimerField_stepTimerStop(&controller->timerField);
        // TimerField_timerEndAfterPulse(&controller->timerField);
        // return;
        goto limit_end_section;
    }
    // 反向限位
    if((controller->mode == MOTOR_TARGET) && 
        controller->leadMotor->targetNegLimit &&
        (controller->leadMotor->current == controller->leadMotor->targetNegLimit))
    {
        // TimerField_stepTimerStop(&controller->timerField);
        // TimerField_timerEndAfterPulse(&controller->timerField);
        // return;
        goto limit_end_section;
    }
    // stop timer and call callback if we reached MOTOR_TARGET
    if((controller->mode == MOTOR_TARGET) && 
       (controller->leadMotor->current == controller->leadMotor->target)){
        // TimerField_stepTimerStop(&controller->timerField);
        // TimerField_timerEndAfterPulse(&controller->timerField);
        // if(controller->reachedTargetCallback)
        //     controller->reachedTargetCallback((int32_t)controller->leadMotor->current);
        goto reached_end_section;
    }
    return;
    limit_end_section:
        TimerField_stepTimerStop(&controller->timerField);
        TimerField_timerEndAfterPulse(&controller->timerField);
        return;
    reached_end_section:
        TimerField_stepTimerStop(&controller->timerField);
        TimerField_timerEndAfterPulse(&controller->timerField);
        if(controller->reachedTargetCallback)
            controller->reachedTargetCallback((int32_t)controller->leadMotor->current);       
        return;
}


void pulseTimerISR(MotorControlBase *controller){
    Stepper* *motor = controller->motorList;
    
    // 在旋转模式并且StepTimer未启动时
    if((controller->mode == MOTOR_NOTARGET) && (!controller->timerField.stepTimerRunning)){
        TimerField_pulseTimerStop(&controller->timerField);
        return;
    }

    while((*motor) != NULL){
        Stepper_clearStepPin((*motor++));
    }

    if(controller->timerField.lastPulse){
        TimerField_end(&controller->timerField);
        // TimerField_pulseTimerStop(&controller->timerField);
        return;
    }
}


