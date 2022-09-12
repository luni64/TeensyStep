
#include "TimerField.h"


static int instances = 0;
static TIM_Module* htim[MAX_TIMERS] = TIM_MODULE();
static TIM_Unit timer_mapping[MAX_TIMERS] = TIMER_UNIT();


TIM_Unit TimerField_getTimer(void){
    // TODO
    return (instances >= MAX_TIMERS) ? timer_mapping[0] : timer_mapping[instances++];
}

void TimerField_init(TimerField *_timerfield, const TimerField_InitTypeDef *config){
    // TODO
    if(config){
        _timerfield->accTimer = config->accTimer;
        _timerfield->stepTimer = config->stepTimer;
        _timerfield->pulseTimer = config->pulseTimer;
    }else{
        _timerfield->stepTimer = TimerField_getTimer();
        _timerfield->accTimer = TimerField_getTimer();
        _timerfield->pulseTimer = TimerField_getTimer();
    }

    TIM_InitTypeDef init = { .TIMx = _timerfield->stepTimer, .TIM_Period = (SYSCTRL->PCLK_1MS_VAL * 1000 / 1000000) };
    TIM_Init(htim[_timerfield->stepTimer], &init);
    init.TIMx = _timerfield->accTimer;
    TIM_Init(htim[_timerfield->accTimer], &init);

    init.TIMx = _timerfield->pulseTimer;
    TIM_Init(htim[_timerfield->pulseTimer], &init);
    // TIM_PWMSinglePulseConfig(htim[_timerfield->pulseTimer], _timerfield->pulseTimer, ENABLE);

    _timerfield->stepTimerRunning = false;
}

bool TimerField_begin(TimerField* timerfield){
    // TODO
    // TIM_ITConfig(htim[timerfield->stepTimer], timerfield->stepTimer, ENABLE);
    // TIM_Cmd(htim[timerfield->stepTimer], timerfield->stepTimer, ENABLE);
    
    timerfield->lastPulse = false;
    return true;
}

void TimerField_end(TimerField* timerfield){
    // TODO
    TimerField_stepTimerStop(timerfield);
    TimerField_accTimerStop(timerfield);
    TimerField_pulseTimerStop(timerfield);
}

void TimerField_stepTimerStart(TimerField* timerfield){
    // TODO
    TIM_ITConfig(htim[timerfield->stepTimer], timerfield->stepTimer, ENABLE);
    TIM_Cmd(htim[timerfield->stepTimer], timerfield->stepTimer, ENABLE);
    
    timerfield->stepTimerRunning = true;
}

void TimerField_stepTimerStop(TimerField* timerfield){
    // TODO
    TIM_Cmd(htim[timerfield->stepTimer], timerfield->stepTimer, DISABLE);
    TIM_ITConfig(htim[timerfield->stepTimer], timerfield->stepTimer, DISABLE);
    if(TIM_GetITStatus(htim[timerfield->stepTimer], timerfield->stepTimer)) TIM_ClearITPendingBit(htim[timerfield->stepTimer], timerfield->stepTimer);

    timerfield->stepTimerRunning = false;
}
bool TimerField_stepTimerIsAllocated(const TimerField* timerfield){
    // TODO
    (void)timerfield;
    return true;
}

bool TimerField_stepTimerIsRunning(const TimerField* timerfield){
    return timerfield->stepTimerRunning;
}

int32_t TimerField_getStepFrequency(const TimerField* timerfield){
    // TODO
    (void)timerfield;
    uint32_t timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;   /// 获取定时器时钟频率
    
    return timer_clk / htim[timerfield->stepTimer]->TIM[timerfield->stepTimer].LoadCount;
}

void TimerField_setStepFrequency(TimerField* timerfield, uint32_t f){   // Hz
    // TODO
    uint32_t timer_clk = 0, period = 0;
    if(f == 0){
        TimerField_stepTimerStop(timerfield);
        return;
    }

    timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;   /// 获取定时器时钟频率
    period = timer_clk / f;

    TIM_SetPeriod(htim[timerfield->stepTimer], timerfield->stepTimer, period);
}

void TimerField_accTimerStart(TimerField* timerfield){
    // TODO
    TIM_ITConfig(htim[timerfield->accTimer], timerfield->accTimer, ENABLE);
    TIM_Cmd(htim[timerfield->accTimer], timerfield->accTimer, ENABLE);

    timerfield->accTimerRunning = true;
}

void TimerField_accTimerStop(TimerField* timerfield){
    // TODO
    TIM_Cmd(htim[timerfield->accTimer], timerfield->accTimer, DISABLE);
    TIM_ITConfig(htim[timerfield->accTimer], timerfield->accTimer, DISABLE);
    if(TIM_GetITStatus(htim[timerfield->accTimer], timerfield->accTimer)) TIM_ClearITPendingBit(htim[timerfield->accTimer], timerfield->accTimer);
    
    timerfield->accTimerRunning = false;
}

void TimerField_setAccUpdatePeriod(TimerField* timerfield, uint32_t _period){  // us
    // TODO
    uint32_t timer_clk = 0, period = 0;
    if(period == 0){
        TimerField_accTimerStop(timerfield);
        return;
    }
    timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;   /// 获取定时器时钟频率
    period = (timer_clk / 1000000) * _period;    

    TIM_SetPeriod(htim[timerfield->accTimer], timerfield->accTimer, period);
}

bool TimerField_accTimerIsRunning(const TimerField* timerfield){
    return timerfield->accTimerRunning;
}

void TimerField_setPulseWidth(TimerField* timerfield, uint32_t delay){  // us
    // TODO
    uint32_t timer_clk = 0, period = 0;
    timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;   /// 获取定时器时钟频率
    period = (timer_clk / 1000000) * delay;    
    
    TIM_SetPeriod(htim[timerfield->pulseTimer], timerfield->pulseTimer, period);
}

void TimerField_triggerDelay(TimerField* timerfield){
    // TODO
    TIM_Cmd(htim[timerfield->pulseTimer], timerfield->pulseTimer, ENABLE);
    TIM_ITConfig(htim[timerfield->pulseTimer], timerfield->pulseTimer, ENABLE);

    timerfield->pulseTimerRunning = true;
}

void TimerField_pulseTimerStop(TimerField* timerfield){
    TIM_Cmd(htim[timerfield->pulseTimer], timerfield->pulseTimer, DISABLE);
    TIM_ITConfig(htim[timerfield->pulseTimer], timerfield->pulseTimer, DISABLE);

    timerfield->pulseTimerRunning = false; 
}

bool TimerField_pulseTimerIsRunning(const TimerField* timerfield)
{
    return timerfield->pulseTimerRunning;
}

void TimerField_timerEndAfterPulse(TimerField *_timerfield){
    // TODO
    _timerfield->lastPulse = true;
}


