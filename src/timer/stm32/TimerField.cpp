#if defined(STM32F4xx)
#include "TimerField.h"
TF_Handler* TimerField::handler;
int TimerField::instances = 0;
TIM_TypeDef* TimerField::timer_mapping[MAX_TIMERS] = { TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12 };
#endif
