#ifndef _STEPPER_H__
#define _STEPPER_H__


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"

typedef uint8_t StepperParamBase;

typedef struct {
    volatile bool dostep;
    volatile uint8_t polarity;
    volatile uint8_t reverse;
    
    volatile int32_t current;
    volatile int32_t currentSpeed;
    volatile int32_t target;
    const int32_t targetPosLimit;
    const int32_t targetNegLimit;

    int32_t dir;

    int32_t A, B; // Bresenham paramters
    int32_t vMax;
    int32_t vPullIn, vPullOut;
    uint32_t a;

    gpio_pin_t stepPin;
    gpio_pin_t dirPin;
}Stepper;

typedef struct {
    // int32_t vMaxMax;
    // uint32_t aMax;
    // uint32_t a;

    gpio_pin_t stepPin;
    gpio_pin_t dirPin;
}Stepper_InitTypeDef;

Stepper* Stepper_init(Stepper *stepper, const Stepper_InitTypeDef *config);

Stepper* Stepper_setMaxSpeed(Stepper* stepper, int32_t speed);                                  // steps/s
Stepper* Stepper_setPullInSpeed(Stepper* stepper, int32_t speed);                               // steps/s
Stepper* Stepper_setPullInOutSpeed(Stepper* stepper, int32_t pullInSpeed, int32_t pullOutSpeed); // steps/s
Stepper* Stepper_setAcceleration(Stepper* stepper, uint32_t _a);                                // steps/s^2

Stepper* Stepper_setStepPinPolarity(Stepper* stepper, int p);  // HIGH -> positive pulses, LOW -> negative pulses
Stepper* Stepper_setInverseRotation(Stepper* stepper, bool b); // Change polarity of the dir pulse

void Stepper_setTargetAbs(Stepper* stepper, int32_t pos);   // Set MOTOR_TARGET position absolute
void Stepper_setTargetRel(Stepper* stepper, int32_t delta); // Set MOTOR_TARGET position relative to current position

static inline int32_t Stepper_getPosition(Stepper* stepper) { return stepper->current; }
static inline void Stepper_setPosition(Stepper* stepper, int32_t pos) { stepper->current = pos; }

/***************************  HardWare Pin  ****************************/


static inline void FUN_IN_RAM Stepper_doStep(Stepper* stepper){
	
    digitalWritePin(stepper->stepPin, stepper->polarity);
    stepper->dostep = true;
    stepper->current += stepper->dir;
}

static inline void FUN_IN_RAM Stepper_clearStepPin(Stepper* stepper){
    digitalWritePin(stepper->stepPin, !stepper->polarity);
    stepper->dostep = false;
}

static inline bool FUN_IN_RAM Stepper_isClearStepPin(Stepper *stepper){
    return !stepper->dostep;
}

static inline void FUN_IN_RAM Stepper_setDir(Stepper* stepper, int d){
    stepper->dir = d;
    digitalWritePin(stepper->dirPin, (stepper->dir == 1) ? stepper->reverse : !stepper->reverse);
}

static inline void FUN_IN_RAM Stepper_toggleDir(Stepper* stepper){
    Stepper_setDir(stepper, -1 * stepper->dir);
}


/******************************  MISC  *********************************/

#include "math.h"
#include "stdlib.h"


// compare functions
static inline bool FUN_IN_RAM Stepper_cmpDelta(const Stepper* a, const Stepper* b) { return a->A > b->A; }
static inline bool FUN_IN_RAM Stepper_cmpAcc(const Stepper* a, const Stepper* b) { return a->a < b->a; }
static inline bool FUN_IN_RAM Stepper_cmpVmin(const Stepper* a, const Stepper* b) { return labs(a->vMax) < labs(b->vMax); }
static inline bool FUN_IN_RAM Stepper_cmpVmax(const Stepper* a, const Stepper* b) { return labs(a->vMax) > labs(b->vMax); }


Stepper** FUN_IN_RAM find_min_element(Stepper* *start, Stepper* *end, bool (*cmp)(const Stepper *, const Stepper *));

void FUN_IN_RAM sort_element(Stepper* *start, Stepper* *end, bool (*cmp)(const Stepper *, const Stepper *));

#endif
