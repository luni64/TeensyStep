


#include <math.h>
#include "Stepper.h"


static int32_t vMaxMax = 300000;   // largest speed possible (steps/s)
static uint32_t aMax = 500000;     // speed up to 500kHz within 1 s (steps/s^2)

static const uint32_t vMaxDefault = 800; // should work with every motor (1 rev/sec in 1/4-step mode)
static const uint32_t vPullInOutDefault = 100;
static const uint32_t aDefault = 2500; // reasonably low (~0.5s for reaching the default speed)


Stepper* Stepper_init(Stepper *stepper, const Stepper_InitTypeDef *config){

    stepper->stepPin = config->stepPin;
    stepper->dirPin = config->dirPin;
    
    digitalPinOutputMode(stepper->stepPin);
    digitalPinOutputMode(stepper->dirPin);
    
	stepper->current = 0;
    Stepper_setStepPinPolarity(stepper, HIGH);
    Stepper_setInverseRotation(stepper, false);
    Stepper_setAcceleration(stepper, aDefault);
    Stepper_setMaxSpeed(stepper, vMaxDefault);
    Stepper_setPullInSpeed(stepper, vPullInOutDefault);

    return stepper;
}

Stepper* Stepper_setMaxSpeed(Stepper* stepper, int32_t speed){  // steps/s
    Stepper_setDir(stepper, (speed >= 0 ? 1 : -1));
    stepper->vMax = min(vMaxMax, max(-vMaxMax, speed));
    
    return stepper;
}
Stepper* Stepper_setPullInSpeed(Stepper* stepper, int32_t speed){  // steps/s
    // (void)stepper;
    stepper->vPullIn = stepper->vPullOut = labs(speed);
    return stepper;
}
Stepper* Stepper_setPullInOutSpeed(Stepper* stepper, int32_t pullInSpeed, int32_t pullOutSpeed){  // steps/s
    
    stepper->vPullIn = labs(pullInSpeed);
    stepper->vPullOut = labs(pullOutSpeed);
    return stepper;
}
Stepper* Stepper_setAcceleration(Stepper* stepper, uint32_t _a){  // steps/s^2
    stepper->a = min(aMax, _a);
    return stepper;
}

Stepper* Stepper_setStepPinPolarity(Stepper* stepper, int p){  // HIGH -> positive pulses, LOW -> negative pulses
    stepper->polarity = p;
    Stepper_clearStepPin(stepper);

    return stepper;
}

Stepper* Stepper_setInverseRotation(Stepper* stepper, bool b){  // Change polarity of the dir pulse
    stepper->reverse = b;
    return stepper;
}

void Stepper_setTargetAbs(Stepper* stepper, int32_t pos){   // Set MOTOR_TARGET position absolute
    
    Stepper_setTargetRel(stepper, pos - stepper->current);
}
void Stepper_setTargetRel(Stepper* stepper, int32_t delta){   // Set MOTOR_TARGET position relative to current position
    
    Stepper_setDir(stepper, (delta < 0) ? -1 : 1);
    stepper->target = stepper->current + delta;
    stepper->A = labs(delta);
}


