
#ifndef _TEENSYSTEP_PORT_H__
#define _TEENSYSTEP_PORT_H__

#include <stdint.h>
#include <stdbool.h>

#ifndef HIGH
#define HIGH             1
#endif
#ifndef LOW
#define LOW              0
#endif


#include "air105_timer.h"
#include "air105_gpio.h"
#include "delay.h"

#ifndef delay
#define delay(ms)       Delay_ms(ms)
#endif


typedef struct {
    uint16_t pin;
    uint32_t port;
}gpio_pin_t;


static inline void digitalPinOutputMode(gpio_pin_t pin){
    // TODO
    GPIO_InitTypeDef gpio = { 0 };
    
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = pin.pin;
    gpio.GPIO_Remap = GPIO_Remap_1;
    GPIO_Init((GPIO_TypeDef *)(pin.port), &gpio);
}

static inline void digitalWritePin(gpio_pin_t pin, uint8_t level){
    // TODO
    uint32_t _pin = pin.pin;
    ((GPIO_TypeDef*)pin.port)->BSRR = ((uint32_t)_pin) << ((!level) * 16);
}

// #include "TF_Handler.h"

#define NO_TIMER             -1
#define AUTO_TIMER           255
#define MAX_TIMERS           7    /// 定义可用定时器单元数目          

#ifndef TIMER_UNIT
#define TIMER_UNIT()          {TIM_1, TIM_2, TIM_3,TIM_4, TIM_5, TIM_6, TIM_7}
#endif

#ifndef TIM_MODULE
#define TIM_MODULE()          {TIMM0, TIMM0, TIMM0, TIMM0, TIMM0, TIMM0, TIMM0}
#endif


typedef TIM_NumTypeDef TIM_Unit; /// 定义定时器单元
typedef TIM_Module_TypeDef TIM_Module;  /// 定义定时器模块

typedef struct {
    volatile bool stepTimerRunning;
    volatile bool lastPulse;
    
    TIM_Unit stepTimer;
    TIM_Unit accTimer;
    TIM_Unit pulseTimer;

    // TF_Handler *handler;
}TimerField;

typedef struct {
    TIM_Unit stepTimer;
    TIM_Unit accTimer;
    TIM_Unit pulseTimer;
}TimerField_InitTypeDef;

void TimerField_init(TimerField *_timerfield, const TimerField_InitTypeDef *config);
bool TimerField_begin(TimerField* timerfield);
void TimerField_end(TimerField* timerfield);

void TimerField_stepTimerStart(TimerField* timerfield);   /// 需要强制更新
void TimerField_stepTimerStop(TimerField* timerfield);
bool TimerField_stepTimerIsRunning(const TimerField* timerfield);
bool TimerField_stepTimerIsAllocated(TimerField* timerfield);
int32_t TimerField_getStepFrequency(const TimerField* timerfield);
void TimerField_setStepFrequency(TimerField* timerfield, uint32_t f);

void TimerField_accTimerStart(TimerField* timerfield);
void TimerField_accTimerStop(TimerField* timerfield);
void TimerField_setAccUpdatePeriod(TimerField* timerfield, uint32_t period);

void TimerField_setPulseWidth(TimerField* timerfield, uint32_t delay);
void TimerField_triggerDelay(TimerField* timerfield);
void TimerField_pulseTimerStop(TimerField* timerfield);

void TimerField_timerEndAfterPulse(TimerField *_timerfield);



#endif
