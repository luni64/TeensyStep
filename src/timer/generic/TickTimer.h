#pragma once

#include <Arduino.h>
#include <functional>
#include <limits>

class TimerControl;

//using callback_t = void (*)();
using callback_t = std::function<void(void)>;

class TimerBase
{
  public:
    TimerBase(callback_t cb, bool _isPeriodic)
    {
        callback = cb;
        deltaCnt = 0;
        run = false;
        isPeriodic = _isPeriodic;
        prev = next = nullptr;
    };

    static inline uint32_t get_cycles(){
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
            return ARM_DWT_CYCCNT;
#else
            return dwt_getCycles();
#endif
    }

    inline void start() { startCnt = get_cycles(); run = true; }
    inline void stop() { run = false; }
    inline bool isRunning() const {return run;}

  protected:
    uint32_t deltaCnt, startCnt;
    bool isPeriodic;
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
        //Serial.println(hz);
        deltaCnt = hz > minFrequency ? F_CPU / hz : std::numeric_limits<uint32_t>::max();
        //Serial.println(deltaCnt);
    }

    inline void setPeriod(uint32_t microSeconds)
    {
        deltaCnt = F_CPU / 1'000'000 * microSeconds;
    }

    static float minFrequency;
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
        deltaCnt = F_CPU / 1'000'000 * microSeconds;
    }
};

class TimerControl
{
  public:
    static void begin()
    {
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
        ARM_DEMCR |= ARM_DEMCR_TRCENA;
        ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
#endif
    }

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
        digitalWrite(2,HIGH) ;
        TimerBase *timer = firstTimer;

        while (timer != nullptr)
        {
            //Serial.printf("cnt: %d\n", ARM_DWT_CYCCNT );

            if (timer->run && (TimerBase::get_cycles() - timer->startCnt >= timer->deltaCnt))

            {
                if (timer->isPeriodic)
                    timer->startCnt = TimerBase::get_cycles();
                else
                    timer->run = false;

                timer->callback();
            }
            timer = timer->next;
        }

        digitalWrite(2,LOW) ;
    }

  protected:
    static TimerBase *firstTimer;
    static TimerBase *lastTimer;
};
