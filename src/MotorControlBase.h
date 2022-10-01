#ifndef _MOTORCONTROLBASE_H__
#define _MOTORCONTROLBASE_H__

#include "ErrorHandler.h"
#include "Stepper.h"
#include "timer/TF_Handler.h"
#include "port/port.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>

#ifndef MAXMOTORS
#define MAXMOTORS         10
#endif

#ifndef ACCUPDATEPERIOD_DEFAULT
#define ACCUPDATEPERIOD_DEFAULT               5
#endif

#ifndef PULSEWIDTH_DEFAULT
#define PULSEWIDTH_DEFAULT                    5000
#endif



typedef enum {
    err_OK,
    err_movment_not_possible,
    err_too_much_motors
}ErrCode;

typedef enum {
    MOTOR_TARGET,
    MOTOR_NOTARGET
}Mode;

typedef void (*ErrFunc)(ErrCode);
typedef void (*CallbackFunc)(int32_t);

typedef struct {
    bool OK;
    uint8_t mCnt;
    Mode mode;
    
    uint32_t accUpdatePeriod;
    uint32_t pulseWidth;

    TimerField timerField;

    Stepper* motorList[MAXMOTORS + 1];
    Stepper* leadMotor;

    CallbackFunc reachedTargetCallback;
}MotorControlBase;


typedef struct {
    uint32_t accUpdatePeriod;
    uint32_t pulseWidth;
    CallbackFunc reachedTargetCallback;
}MotorControlBase_Init_TypeDef;

void Error(ErrCode e);
void attachErrorFunction(ErrFunc ef);

static inline mcErr err(mcErr code){
    return (mcErr)error(eM_MC, (int)code);
}


static inline bool Controller_isOK(const MotorControlBase * controller){

    return controller->OK;
}

MotorControlBase* Controller_init(MotorControlBase* controller, const MotorControlBase_Init_TypeDef *config);

static inline bool Controller_isRunning(const MotorControlBase *controller){
    return TimerField_stepTimerIsRunning(&controller->timerField);
}

static inline bool Controller_isAllocated(const MotorControlBase *controller){
    return TimerField_stepTimerIsAllocated(&controller->timerField);
}

int32_t Controller_getCurrentSpeed(const MotorControlBase *controller);

static inline void Controller_emergencyStop(MotorControlBase *controller){
    TimerField_end(&controller->timerField);
}

static inline void Controller_stop(MotorControlBase* controller){
    Controller_emergencyStop(controller);
}

void Controller_attachStepper(MotorControlBase *controller, uint8_t N, Stepper* *steppers);

void vController_attachStepper(MotorControlBase *controller, uint8_t N, ...);

void vvController_attachStepper(MotorControlBase *controller, uint8_t N, __builtin_va_list va);



void FUN_IN_RAM stepTimerISR(MotorControlBase *controller);

void FUN_IN_RAM pulseTimerISR(MotorControlBase *controller);


#endif
