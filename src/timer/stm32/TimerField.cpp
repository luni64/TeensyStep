#if defined(STM32F4xx)
#include "TimerField.h"
int TimerField::instances = 0;
TIM_TypeDef* TimerField::timer_mapping[MAX_TIMERS] = { TIM1, TIM2, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM14 };
// TIM3 and TIM13 used by HAL/FreeRTOS? doesn't work well to use.
#endif
