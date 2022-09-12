#ifndef _ROTATECONTROLBASE_H__
#define _ROTATECONTROLBASE_H__

#include "MotorControlBase.h"
#include "accelerators/LinRotAccelerator.h"


typedef MotorControlBase_Init_TypeDef RotateControl_Init_TypeDef;


typedef struct {
    MotorControlBase controller;
    LinRotAccelerator accelerator;
}RotateControl;


void RotateControl_init(RotateControl *controller, const RotateControl_Init_TypeDef *config);

// Non-blocking movements ----------------

void RotateControl_rotateAsync(RotateControl *_controller, float speedFactor, uint8_t N, Stepper* *steppers);

void vRotateControl_rotateAsync(RotateControl *_controller, float speedFactor, uint8_t N, ...);

void RotateControl_stopAsync(RotateControl *controller);

// Blocking movements --------------------
void RotateControl_stop(RotateControl *controller);

void RotateControl_overrideSpeed(RotateControl *controller, float speedFac);
void RotateControl_overrideAcceleration(RotateControl *controller, float accFac);



void RotateControl_accTimerISR(RotateControl *_controller);

#endif
