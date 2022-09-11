
#include "StepControlBase.h"
#include "TimerField.h"

static Stepper** find_min_element(Stepper* *start, Stepper* *end, bool (*cmp)(const Stepper *, const Stepper *)){
    Stepper* * next = start;
    for(int i = 1; i < ((end - start)); i++){
        if(cmp(start[i], *next)){
            next = &start[i];
        }
    }
    return next;
}


static void sort_element(Stepper* *start, Stepper* *end, bool (*cmp)(const Stepper *, const Stepper *)){
    int l = end - start;
    Stepper *swap = NULL;

    // 冒泡排序
    for(int i = 0; i < (l - 1); i++){
        for(int j = 0; j < (l - i - 1); j++){
            if(cmp(start[j + 1], start[j])){
                swap = start[j + 1];
                start[j + 1] = start[j];
                start[j] = swap;
            }
        }
    }
}


static void doMove(StepControl *_controller, int N, float speedOverride){
    MotorControlBase *controller = &_controller->controller;

    LinStepAccelerator* accelerator = &_controller->accelerator;

    //Calculate Bresenham parameters -------------------------------------
    sort_element(controller->motorList, controller->motorList + N, Stepper_cmpDelta); // The motor which does most steps leads the movement, move to top of list

    controller->leadMotor = controller->motorList[0];

    for (int i = 1; i < N; i++)
    {
        controller->motorList[i]->B = 2 * controller->motorList[i]->A - controller->leadMotor->A;
    }

    // Calculate acceleration parameters --------------------------------
    uint32_t pullInSpeed = controller->leadMotor->vPullIn;
    uint32_t pullOutSpeed = controller->leadMotor->vPullOut;

    uint32_t acceleration = (*(find_min_element(controller->motorList, controller->motorList + N, Stepper_cmpAcc)))->a; // use the lowest acceleration for the move

    uint32_t targetSpeed = labs((*(find_min_element(controller->motorList, controller->motorList + N, Stepper_cmpVmin)))->vMax) * speedOverride; // use the lowest max frequency for the move, scale by relSpeed

    if ((controller->leadMotor->A == 0) || (targetSpeed == 0)) return;

    // target speed----

    float x = 0;
    float leadSpeed = labs(controller->leadMotor->vMax);
    for (int i = 0; i < N; i++)
    {
        float relDist = controller->motorList[i]->A / (float)controller->leadMotor->A * leadSpeed / labs(controller->motorList[i]->vMax);
        if (relDist > x) x = relDist;
        // Serial.printf("%d %f\n", i, relDist);
    }
    targetSpeed = leadSpeed / x;
    //Serial.printf("\n%d\n",targetSpeed);

    // Start move--------------------------
    timerBegin(&controller->timerField);
    int32_t freq = Accelerator_prepareMovement(accelerator, controller->leadMotor->current, controller->leadMotor->target, targetSpeed, pullInSpeed, pullOutSpeed, acceleration);

    setStepFrequency(&controller->timerField, freq);

    stepTimerStart(&controller->timerField);
    accTimerStart(&controller->timerField);
}


void StepControl_moveAsync(StepControl *_controller, float speedOverride, uint8_t N, Stepper* *steppers){
    MotorControlBase *controller = &_controller->controller;
    
    Controller_attachStepper(controller, N, steppers);
    doMove(_controller, N, speedOverride);
}

void vStepControl_moveAsync(StepControl *_controller, float speedOverride, uint8_t N, ...){
    MotorControlBase *controller = &_controller->controller;
    va_list mlist;
    Stepper* stepperArr[MaxMotors] = { NULL };

    va_start(mlist, N);
    for(int i = 0; i < N; i++){
        stepperArr[i] = va_arg(mlist, Stepper *);
    }
    va_end(mlist);

    Controller_attachStepper(controller, N, stepperArr);
    doMove(_controller, N, speedOverride);
}

// non blocking stop command
void StepControl_stopAsync(StepControl *_controller){
    MotorControlBase *controller = &_controller->controller;
    LinStepAccelerator *accelerator = &_controller->accelerator;

    if(Controller_isRunning(controller)){
        uint32_t newTarget = Accelerator_initiateStopping(accelerator, controller->leadMotor->current);

        controller->leadMotor->target = controller->leadMotor->current + controller->leadMotor->dir * newTarget;

        if(controller->leadMotor->target == controller->leadMotor->current){
            timerEnd(&controller->timerField);
        }
    }
    
}


void StepControl_move(StepControl *_controller, float speedOverride, uint8_t N, Stepper* *steppers){
    MotorControlBase *controller = &_controller->controller;

    StepControl_moveAsync(_controller, speedOverride, N, steppers);

    while(stepTimerIsRunning(&controller->timerField)){
        delay(1);
    }
}

void vStepControl_move(StepControl *_controller, float speedOverride, uint8_t N, ...){
    MotorControlBase *controller = &_controller->controller;
    va_list mlist;
    Stepper* stepperArr[MaxMotors] = { NULL };

    va_start(mlist, N);
    for(int i = 0; i < N; i++){
        stepperArr[i] = va_arg(mlist, Stepper *);
    }
    va_end(mlist);

    StepControl_moveAsync(_controller, speedOverride, N, stepperArr);

    while(stepTimerIsRunning(&controller->timerField)){
        delay(1);
    }
}

// blocking stop command
void StepControl_stop(StepControl *_controller){
    MotorControlBase *controller = &_controller->controller;

    StepControl_stop(_controller);
    while(Controller_isRunning(controller)){
        delay(1);
    }
}


void stepTimerISR(StepControl *_controller){
    
    MotorControlBase *controller = &_controller->controller;

    doStep(controller->leadMotor);  /// leadMotor=MotorList[0]

    Stepper* *slave = controller->motorList;
    
    // move slave motors if required (https://en.wikipedia.org/wiki/Bresenham)
    while(*(++slave) != NULL){  // Skip MotorList[0]
        if((*slave)->B >= 0){
            doStep(*slave);
            (*slave)->B -= controller->leadMotor->A;
        }
        (*slave)->B += (*slave)->A;
    }

    triggerDelay(&controller->timerField);  // start delay line to dactivate all step pins
    
    // stop timer and call callback if we reached target
    if((controller->mode == target) && (controller->leadMotor->current == controller->leadMotor->target)){
        // stepTimerStop(&controller->timerField);
        timerEndAfterPulse(&controller->timerField);
        if(controller->reachedTargetCallback)
            controller->reachedTargetCallback((int32_t)controller->leadMotor->current);
    }
    if(controller->timerField.lastPulse){
        stepTimerStop(&controller->timerField);
    }
}


void pulseTimerISR(StepControl *_controller){
    MotorControlBase *controller = &_controller->controller;
    Stepper* *motor = controller->motorList;

    while((*motor) != NULL){
        clearStepPin((*motor++));
    }
    if(controller->timerField.lastPulse){
        // timerEnd(&controller->timerField);
        pulseTimerStop(&controller->timerField);
    }
}

void accTimerISR(StepControl *_controller){
    MotorControlBase *controller = &_controller->controller;
    
    LinStepAccelerator *accelerator = &_controller->accelerator;
    
    int32_t speed = 0;

    if(Controller_isRunning(controller)){
        speed = Accelerator_updateSpeed(accelerator, controller->leadMotor->current);
        setStepFrequency(&controller->timerField, speed);
    }
    if(controller->timerField.lastPulse){
        // timerEnd(&controller->timerField);
        accTimerStop(&controller->timerField);
    }
}

