


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



static inline void swap(Stepper* *a, Stepper* *b){
    Stepper* t = *a;
    *a = *b;
    *b = t;
}

static int partition(Stepper* *arr, int low, int high, bool (*cmp)(const Stepper *, const Stepper *)){
    const Stepper* pivot = arr[high];
    int i = (low - 1);
    for(int j = low; j <= high - 1; j++){
        if(cmp(arr[j], pivot)){
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

static void quick_sort(Stepper* *arr, int low, int high, bool (*cmp)(const Stepper *, const Stepper *)){
    if(low < high){
        int pi = partition(arr, low, high, cmp);
        quick_sort(arr, low, pi - 1, cmp);
        quick_sort(arr, pi + 1, high, cmp);
    }
}

Stepper** find_min_element(Stepper* *start, Stepper* *end, bool (*cmp)(const Stepper *, const Stepper *)){

    Stepper* * next = start;
    for(int i = 1; i < ((end - start)); i++){
        if(cmp(start[i], *next)){
            next = &start[i];
        }
    }

    return next;
}


void sort_element(Stepper* *start, Stepper* *end, bool (*cmp)(const Stepper *, const Stepper *)){

#if(0)
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
#endif

    int n = end - start;
    quick_sort(start, 0, n - 1, cmp);
}



