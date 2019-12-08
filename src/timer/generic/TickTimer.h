#pragma once

// Including main.h ensures that the correct HAL headers are included.
// STM32Step assumes the use of STM32Cube environment.
extern "C" {
#include "tim.h"
}

#include <functional>
#include <limits>

class TimerControl;

//using callback_t = void (*)();
using callback_t = std::function<void(void)>;

/**
 * TimerBase is an abstraction above the TimerControl class.
 * Represents a user defined timer instance.
 * */
class TimerBase
{
  public:
    TimerBase(callback_t cb, bool _isPeriodic)
    {
        callback = cb;
        deltaCnt = 0;
        started = false;
        run = false;
        isPeriodic = _isPeriodic;
        prev = next = nullptr;
    };

    inline void start() { started = false; run = true; }
    inline void stop() { started = false; run = false; }
    inline bool isRunning() const {return run;}

  protected:
    uint32_t deltaCnt, startCnt;
    bool isPeriodic;
    bool started;
    bool run;
    callback_t callback;

    TimerBase *prev, *next;

    friend TimerControl;
};

class PeriodicTimer : public TimerBase
{
  public:
    PeriodicTimer(callback_t cb) : TimerBase(cb, true) 
    {
        
    }

    inline void setFrequency(float hz)
    {
        deltaCnt = hz > minFrequency ? F_CPU / hz : std::numeric_limits<uint32_t>::max();
    }

    inline void setPeriod(uint32_t microSeconds)
    {
        static_assert((F_CPU % 1'000'000) == 0, "CPU frequency is not a multiple of 1 MHz as assumed");
        deltaCnt = F_CPU / 1'000'000 * microSeconds;
    }

    static constexpr float minFrequency = (float)F_CPU / std::numeric_limits<uint32_t>::max();
};

class OneShotTimer : public TimerBase
{
  public:
    OneShotTimer(callback_t cb, unsigned delay = 0) : TimerBase(cb, false) 
    {
        setDelay(delay);
    }

    void setDelay(unsigned microSeconds)
    {
        static_assert((F_CPU % 1'000'000) == 0, "CPU frequency is not a multiple of 1 MHz as assumed");
        deltaCnt = F_CPU / 1'000'000 * microSeconds;
    }
};

/**
 * TimerControl handles the actual timer implementation (in hw/hal).
 * This version supports STM32 HAL implementation.
 * */
class TimerControl
{
  public:
    static void begin()
    {
        static_assert((F_CPU % 1'000'000) == 0, "CPU frequency is not a multiple of 1 MHz as assumed");

        // very temporary solution
        TimerControl::htim = &htim2;

        // initialize 100 kHz timer interrupt generation
        htim->Init.Prescaler = F_CPU / 1'000'000 - 1; // ensure 1 MHz timer frequency
        htim->Init.Period = 10 - 1; // divide 1 MHz clock to 100 kHz
        htim->Init.CounterMode = TIM_COUNTERMODE_UP;
        htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
        htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        

        HAL_TIM_Base_Init(htim);
        HAL_TIM_Base_Start_IT(htim);
    }

    /**
     * WARNING!!! THIS FUNCTION IS NOT INTERRUPT SAFE!!!
     * */
    static void attachTimer(TimerBase *timer)
    {
        if (timer == nullptr)
            return;

        if (firstTimer == nullptr) // empty timer list
        {
            firstTimer = lastTimer = timer;
            timer->prev = nullptr;
            timer->next = nullptr;
        }
        else // append timer at end of the list
        {
            timer->next = nullptr;
            lastTimer->next = timer;
            lastTimer = timer;
        }
    }

    /**
     * WARNING!!! THIS FUNCTION IS NOT INTERRUPT SAFE!!!
     * */
    static void detachTimer(TimerBase *timer)
    {
        if (firstTimer == nullptr || timer == nullptr)
            return;

        TimerBase *t = firstTimer;
        while (t != timer)
        {
            t = t->next;
            if (t == nullptr)
                return;
        }

        if (t == firstTimer)
        {
            firstTimer = t->next;
        }
        else if (t == lastTimer)
        {
            lastTimer = t->prev;
        }
        else
        {
            t->prev->next = t->next;
        }
    }

    static inline void tick()
    {
        TimerBase *timer = firstTimer;
        
        static uint32_t cnt = 0;
        cnt += F_CPU / 100'000;
        
        while (timer != nullptr)
        {
            if (timer->run){

                // start counter
                if (!timer->started){
                    timer->startCnt = cnt;
                    timer->started = true;
                }

                // handle elapsed period
                if (cnt - timer->startCnt >= timer->deltaCnt) {
                    if (timer->isPeriodic)
                        timer->startCnt = cnt;
                    else
                        timer->run = false;

                    timer->callback();
                }
            }
            timer = timer->next;
        }
    }
    
    static inline void tick_ISR(TIM_HandleTypeDef* htim)
    {
        // prevent execution on other timers interrupts
        if (htim != TimerControl::htim) return;

        //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // LED on NUCLEO-F303RE

        TimerControl::tick();

        //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // LED on NUCLEO-F303RE
    }

  protected:
    static TimerBase *firstTimer;
    static TimerBase *lastTimer;
    static TIM_HandleTypeDef* htim;
};

