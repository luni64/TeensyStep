

#include "LinStepAccelerator.h"

typedef struct {
    int32_t s_0;
    int32_t ds;
    uint32_t vs;
    uint32_t ve;
    uint32_t vt;
    int64_t vs_sqr;
    int64_t ve_sqr;
    int64_t vt_sqr;
    uint32_t two_a;
    int32_t accEnd;
    int32_t decStart;
}LinStepAcceleratorParam;


int32_t Accelerator_prepareMovement(LinStepAccelerator *_accelerator, int32_t currentPos, int32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t pullOutSpeed, uint32_t a){
    
    LinStepAcceleratorParam *accelerator = (LinStepAcceleratorParam*)_accelerator->param;

    accelerator->vt = targetSpeed;
    accelerator->vs = pullInSpeed;  // v_start
    accelerator->ve = pullOutSpeed; // v_end
    accelerator->two_a = 2 * a;

    accelerator->s_0 = currentPos;
    accelerator->ds = labs(targetPos - currentPos);

    accelerator->vs_sqr = accelerator->vs * accelerator->vs;
    accelerator->ve_sqr = accelerator->ve * accelerator->ve;
    accelerator->vt_sqr = accelerator->vt * accelerator->vt;

    int32_t sm = ((accelerator->ve_sqr - accelerator->vs_sqr) / accelerator->two_a + accelerator->ds) / 2; // position where acc and dec curves meet

    // Serial.printf("ve: %d\n", ve);
    // Serial.printf("vs: %d\n", vs);
    // Serial.printf("ds: %d\n", ds);
    // Serial.printf("sm: %i\n", sm);

    if (sm >= 0 && sm <= accelerator->ds) // we can directly reach the MOTOR_TARGET with the given values vor v0, ve and a
    {
        int32_t sa = (accelerator->vt_sqr - accelerator->vs_sqr) / accelerator->two_a; // required distance to reach MOTOR_TARGET speed
        if (sa < sm)                              // MOTOR_TARGET speed can be reached
        {
            accelerator->accEnd = sa;
            accelerator->decStart = sm + (sm - sa);
            //Serial.printf("reachable accEnd: %i decStart:%i\n", accEnd, decStart);
        }
        else
        {
            accelerator->accEnd = accelerator->decStart = sm;
            //Serial.printf("limit accEnd: %i decStart:%i\n", accEnd, decStart);
        }
    }
    else
    {
        // hack, call some error callback instead
        while (1)
        {
            // digitalToggle(LED_BUILTIN);
            // delay(25);
        }
    }
    return accelerator->vs;
}

int32_t FUN_IN_RAM Accelerator_updateSpeed(LinStepAccelerator *_accelerator, int32_t currentPosition){

    LinStepAcceleratorParam *accelerator = (LinStepAcceleratorParam*)_accelerator->param;
    int32_t s = labs(accelerator->s_0 - currentPosition);

    // acceleration phase -------------------------------------
    if (s < accelerator->accEnd)
    {
        return sqrtf(accelerator->two_a * s + accelerator->vs_sqr);
    }

    // constant speed phase ------------------------------------
    if (s < accelerator->decStart)
    {
        return accelerator->vt;
    }

    //deceleration phase --------------------------------------
    if (s < accelerator->ds)
    {
        //  return sqrtf(two_a * ((stepsDone < ds - 1) ? ds - stepsDone - 2 : 0) + vs_sqr);
        return sqrtf(accelerator->ve_sqr + (accelerator->ds - s - 1) * accelerator->two_a);
    }

    //we are done, make sure to return 0 to stop the step timer
    return 0;

}

uint32_t Accelerator_initiateStopping(LinStepAccelerator *_accelerator, int32_t currentPosition){

    LinStepAcceleratorParam *accelerator = (LinStepAcceleratorParam*)_accelerator->param;
   int32_t stepsDone = labs(accelerator->s_0 - currentPosition);

    if (stepsDone < accelerator->accEnd) // still accelerating
    {
        accelerator->accEnd = accelerator->decStart = 0; // start deceleration
        accelerator->ds = 2 * stepsDone;    // we need the same way to decelerate as we traveled so far
        return stepsDone;      // return steps to go
    }
    else if (stepsDone < accelerator->decStart) // constant speed phase
    {
        accelerator->decStart = 0;            // start deceleration
        accelerator->ds = stepsDone + accelerator->accEnd; // normal deceleration distance
        return accelerator->accEnd;           // return steps to go
    }
    else // already decelerating
    {
        return accelerator->ds - stepsDone; // return steps to go
    }
}

void Accelerator_overrideSpeed(LinStepAccelerator *_accelerator, float fac, int32_t currentPosition){
    (void)_accelerator;
    (void)fac;
    (void)currentPosition;
}

