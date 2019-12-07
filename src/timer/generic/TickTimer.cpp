#include "TickTimer.h"

void std::__throw_bad_function_call()
{
    while(1);
}

TimerBase* TimerControl::firstTimer = nullptr;
TimerBase* TimerControl::lastTimer = nullptr;
TIM_HandleTypeDef* TimerControl::htim = nullptr;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){
    TimerControl::tick_ISR(htim);
}