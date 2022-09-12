

#include "RotateControlBase.h"
#include "TimerField.h"

// Implementation *************************************************************************************************

static void doRotate(RotateControl *_controller, int N, float speedFactor)
{
    MotorControlBase *controller = (MotorControlBase*)&_controller->controller;
    if(Controller_isRunning(controller))
    {
        // this->err(mcErr::alrdyMoving);
        return;
    }

    //Calculate Bresenham parameters ----------------------------------------------------------------
    sort_element(controller->motorList, controller->motorList + N, Stepper_cmpVmax);
    controller->leadMotor = controller->motorList[0];

    if (controller->leadMotor->vMax == 0)
        return;

    controller->leadMotor->currentSpeed = 0;

    controller->leadMotor->A = labs(controller->leadMotor->vMax);
    for (int i = 1; i < N; i++)
    {
        controller->motorList[i]->A = labs(controller->motorList[i]->vMax);
        controller->motorList[i]->B = 2 * controller->motorList[i]->A - controller->leadMotor->A;
    }
    uint32_t acceleration = (*(find_min_element(controller->motorList, controller->motorList + N, Stepper_cmpAcc)))->a; // use the lowest acceleration for the move

    // Start moving----------------------------------------------------------------------------------------------

    TimerField_begin(&controller->timerField);
    RotAccelerator_prepareRotation(controller->leadMotor->current, controller->leadMotor->vMax, acceleration, controller->accUpdatePeriod, speedFactor);
    TimerField_accTimerStart(&controller->timerField);
}


void RotateControl_init(RotateControl *controller, const RotateControl_Init_TypeDef *config)
{
    Controller_init(&controller->controller, config);
    controller->controller.mode = MOTOR_NOTARGET;
}


// ISR -----------------------------------------------------------------------------------------------------------


void RotateControl_accTimerISR(RotateControl *_controller)
{
    MotorControlBase *controller = (MotorControlBase*)&_controller->controller;
    LinRotAccelerator *acceletator = (LinRotAccelerator*)&_controller->accelerator;

    int32_t newSpeed = RotAccelerator_updateSpeed(acceletator, controller->leadMotor->current); // get new speed for the leading motor

    if(newSpeed == 0)
    {
        TimerField_end(&controller->timerField);
        controller->leadMotor->currentSpeed = 0;
        return;
    }

    if (controller->leadMotor->currentSpeed == newSpeed)
    {
        return; // nothing changed, just keep running
    }

    int dir = newSpeed >= 0 ? 1 : -1; // direction changed? -> toggle direction of all motors
    if (dir != controller->leadMotor->dir)
    {
        Stepper** motor = controller->motorList;
        while ((*motor) != NULL)
        {
            // (*motor++)->toggleDir();
            Stepper_toggleDir((*motor++));
        }
        while();
        // delayMicroseconds(controller->pulseWidth); // dir signal need some lead time
        TimerField_setPulseWidth(&controller->timerField, controller->pulseWidth);
        TimerField_triggerDelay(&controller->timerField);
        while(TimerField_pulseTimerIsRunning(&controller->timerField));
    }

    TimerField_setStepFrequency(&controllrt->timerField, labs(newSpeed)); // speed changed, update timer
    controller->leadMotor->currentSpeed = newSpeed;
}

// ROTATE Commands -------------------------------------------------------------------------------

void RotateControl_rotateAsync(RotateControl *_controller, float speedFactor, uint8_t N, ...)
{
    MotorControlBase *controller = &_controller->controller;
    va_list mlist;

    va_start(mlist, N);
    vvController_attachStepper(controller, N, mlist);
    va_end(mlist);

    doRotate(_controller, N, speedFactor);
}


void RotateControl_rotateAsync(RotateControl *_controller, float speedFactor, uint8_t N, Stepper* *steppers)
{
    MotorControlBase *controller = &_controller->controller;

    Controller_attachStepper(controller, N, steppers);

    doRotate(_controller, N, speedFactor);
}


void RotateControl_overrideSpeed(RotateControl *controller, float factor)
{
    // isStopping = false;
    LinRotAccelerator *accelerator = &controller->accelerator;
    RotAccelerator_overrideSpeed(accelerator, factor);
}


void RotateControl_overrideAcceleration(RotateControl *controller, float factor)
{
    LinRotAccelerator *accelerator = &controller->accelerator;

    RotAccelerator_overrideAcceleration(accelerator, factor);
}


void RotateControl_stopAsync(RotateControl *controller)
{
    // if (!isStopping)
    // {
        // isStopping = true;
    MotorControlBase *controller = &_controller->controller;
    LinRotAccelerator *accelerator = &controller->accelerator;
    
    if(TimerField_accTimerIsRunning(&controllrt->timerField) == false)
        return;

    RotAccelerator_initiateStopping(accelerator, controller->leadMotor->current);

    TimerField_timerEndAfterPulse(&controller->timerField);
    // }
}

void RotateControl_stop(RotateControl *_controller)
{
    const MotorControlBase *controller = &_controller->controller;

    RotateControl_stopAsync(_controller);
    while (Controller_isRunning(controller))
    {
        delay(1);
    }
}

