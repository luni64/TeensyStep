#if defined(STM32F4xx)

#include "TimerField.h"

int TimerField::instances = 0;
// TIM3 and TIM13 used by HAL/FreeRTOS? doesn't work well to use.
TIM_TypeDef* TimerField::timer_mapping[MAX_TIMERS] = {
#ifdef TIM1
    TIM1,
#else
    nullptr,
#endif
#ifdef TIM2
    TIM2,
#else
    nullptr,
#endif
#ifdef TIM3
    nullptr, //TIM3,
#else
    nullptr,
#endif
#ifdef TIM4
    TIM4,
#else
    nullptr,
#endif
#ifdef TIM5
    TIM5,
#else
    nullptr,
#endif
#ifdef TIM6
    TIM6,
#else
    nullptr,
#endif
#ifdef TIM7
    TIM7,
#else
    nullptr,
#endif
#ifdef TIM8
    TIM8,
#else
    nullptr,
#endif
#ifdef TIM9
    TIM9,
#else
    nullptr,
#endif
#ifdef TIM10
    TIM10,
#else
    nullptr,
#endif
#ifdef TIM11
    TIM11,
#else
    nullptr,
#endif
#ifdef TIM12
    TIM12,
#else
    nullptr,
#endif
#ifdef TIM13
    nullptr, //TIM13,
#else
    nullptr,
#endif
#ifdef TIM14
    TIM14,
#else
    nullptr,
#endif
#ifdef TIM15
    TIM15,
#else
    nullptr,
#endif
#ifdef TIM16
    TIM16,
#else
    nullptr,
#endif
#ifdef TIM17
    TIM17,
#else
    nullptr,
#endif
#ifdef TIM18
    TIM18,
#else
    nullptr,
#endif
#ifdef TIM19
    TIM19,
#else
    nullptr,
#endif
#ifdef TIM20
    TIM20,
#else
    nullptr,
#endif
#ifdef TIM21
    TIM21,
#else
    nullptr,
#endif
#ifdef TIM22
    TIM22
#else
    nullptr
#endif
};
#endif
