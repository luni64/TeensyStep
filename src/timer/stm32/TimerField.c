
#include "TimerField.h"


static int instances = 0;
static TIM_Module* htim[MAX_TIMERS] = TIM_MODULE();
static TIM_Unit timer_mapping[MAX_TIMERS] = TIMER_UNIT();


TIM_Unit get_timer(void){
    // TODO
    return (instances >= MAX_TIMERS) ? timer_mapping[0] : timer_mapping[instances++];
}

void timerInit(TimerField *_timerfield, const TimerField_InitTypeDef *config){
    // TODO
    if(config){
        _timerfield->accTimer = config->accTimer;
        _timerfield->stepTimer = config->stepTimer;
        _timerfield->pulseTimer = config->pulseTimer;
    }else{
        _timerfield->stepTimer = get_timer();
        _timerfield->accTimer = get_timer();
        _timerfield->pulseTimer = get_timer();
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

bool timerBegin(TimerField* timerfield){
    // TODO
    // TIM_ITConfig(htim[timerfield->stepTimer], timerfield->stepTimer, ENABLE);
    // TIM_Cmd(htim[timerfield->stepTimer], timerfield->stepTimer, ENABLE);
    
    timerfield->lastPulse = false;
    return true;
}

void timerEnd(TimerField* timerfield){
    // TODO
    stepTimerStop(timerfield);
    accTimerStop(timerfield);
    pulseTimerStop(timerfield);
}

void stepTimerStart(TimerField* timerfield){
    // TODO
    TIM_ITConfig(htim[timerfield->stepTimer], timerfield->stepTimer, ENABLE);
    TIM_Cmd(htim[timerfield->stepTimer], timerfield->stepTimer, ENABLE);
    
    timerfield->stepTimerRunning = true;
}

void stepTimerStop(TimerField* timerfield){
    // TODO
    TIM_Cmd(htim[timerfield->stepTimer], timerfield->stepTimer, DISABLE);
    TIM_ITConfig(htim[timerfield->stepTimer], timerfield->stepTimer, DISABLE);
    if(TIM_GetITStatus(htim[timerfield->stepTimer], timerfield->stepTimer)) TIM_ClearITPendingBit(htim[timerfield->stepTimer], timerfield->stepTimer);

    timerfield->stepTimerRunning = false;
}
bool stepTimerIsAllocated(TimerField* timerfield){
    // TODO
    (void)timerfield;
    return true;
}

bool stepTimerIsRunning(const TimerField* timerfield){
    return timerfield->stepTimerRunning;
}

int32_t getStepFrequency(const TimerField* timerfield){
    // TODO
    (void)timerfield;
    uint32_t timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;   /// 获取定时器时钟频率
    
    return timer_clk / htim[timerfield->stepTimer]->TIM[timerfield->stepTimer].LoadCount;
}

void setStepFrequency(TimerField* timerfield, uint32_t f){   // Hz
    // TODO
    uint32_t timer_clk = 0, period = 0;
    if(f == 0){
        stepTimerStop(timerfield);
        return;
    }

    timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;   /// 获取定时器时钟频率
    period = timer_clk / f;

    TIM_SetPeriod(htim[timerfield->stepTimer], timerfield->stepTimer, period);
}

void accTimerStart(TimerField* timerfield){
    // TODO
    TIM_ITConfig(htim[timerfield->accTimer], timerfield->accTimer, ENABLE);
    TIM_Cmd(htim[timerfield->accTimer], timerfield->accTimer, ENABLE);
}

void accTimerStop(TimerField* timerfield){
    // TODO
    TIM_Cmd(htim[timerfield->accTimer], timerfield->accTimer, DISABLE);
    TIM_ITConfig(htim[timerfield->accTimer], timerfield->accTimer, DISABLE);
    if(TIM_GetITStatus(htim[timerfield->accTimer], timerfield->accTimer)) TIM_ClearITPendingBit(htim[timerfield->accTimer], timerfield->accTimer);
}

void setAccUpdatePeriod(TimerField* timerfield, uint32_t _period){  // us
    // TODO
    uint32_t timer_clk = 0, period = 0;
    if(period == 0){
        accTimerStop(timerfield);
        return;
    }
    timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;   /// 获取定时器时钟频率
    period = (timer_clk / 1000000) * _period;    

    TIM_SetPeriod(htim[timerfield->accTimer], timerfield->accTimer, period);
}

void setPulseWidth(TimerField* timerfield, uint32_t delay){  // us
    // TODO
    uint32_t timer_clk = 0, period = 0;
    timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;   /// 获取定时器时钟频率
    period = (timer_clk / 1000000) * delay;    
    
    TIM_SetPeriod(htim[timerfield->pulseTimer], timerfield->pulseTimer, period);
}

void triggerDelay(TimerField* timerfield){
    // TODO
    TIM_Cmd(htim[timerfield->pulseTimer], timerfield->pulseTimer, ENABLE);
    TIM_ITConfig(htim[timerfield->pulseTimer], timerfield->pulseTimer, ENABLE);
}

void pulseTimerStop(TimerField* timerfield){
    TIM_Cmd(htim[timerfield->pulseTimer], timerfield->pulseTimer, DISABLE);
    TIM_ITConfig(htim[timerfield->pulseTimer], timerfield->pulseTimer, DISABLE);   
}

void timerEndAfterPulse(TimerField *_timerfield){
    // TODO
    _timerfield->lastPulse = true;
}


