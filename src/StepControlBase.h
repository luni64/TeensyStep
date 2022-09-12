#ifndef _STEPCONTROLBASE_H__
#define _STEPCONTROLBASE_H__

#include "MotorControlBase.h"
#include "accelerators/LinStepAccelerator.h"

typedef MotorControlBase_Init_TypeDef StepControl_Init_TypeDef;

typedef struct {
    MotorControlBase controller;
    LinStepAccelerator accelerator;
}StepControl;


static inline void StepControl_init(StepControl *controller, const StepControl_Init_TypeDef *config){
    // TODO
    Controller_init(&controller->controller, config);
    controller->controller.mode = MOTOR_TARGET;
}

 // Non-blocking movements ------------------------------------------

void StepControl_moveAsync(StepControl *_controller, float speedOverride, uint8_t N, Stepper* *steppers);

/**
 * @brief 
 * 
 * @param _controller 
 * @param speedOverride 
 * @param N 
 * @param ... va_list 类型是 Stepper*
 */
void vStepControl_moveAsync(StepControl *_controller, float speedOverride, uint8_t N, ...);

// non blocking stop command
void StepControl_stopAsync(StepControl *_controller);

// Blocking movements -------------------------------------------

void StepControl_move(StepControl *_controller, float speedOverride, uint8_t N, Stepper* *steppers);

void vStepControl_move(StepControl *_controller, float speedOverride, uint8_t N, ...);

// blocking stop command
void StepControl_stop(StepControl *_controller);

static inline void StepControl_setReachedTargetCallback(StepControl *_controller, CallbackFunc cb){

    _controller->controller.reachedTargetCallback = cb;
}


/**
 * @brief step 定时器回调函数, 此函数应该在定时器 UPDATE ISR 中调用
 * 
 * @param _controller 
 */
void stepTimerISR(StepControl *_controller);

void pulseTimerISR(StepControl *_controller);

void accTimerISR(StepControl *_controller);


#endif
