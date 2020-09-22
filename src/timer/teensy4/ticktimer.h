#if defined(__IMXRT1052__)
#include <functional>
#include "Arduino.h"
//#include <vector>

using callback_t = void (*)();
//using callback_t = std::function<void(void)>;

struct channelInfo
{
    channelInfo()
    {
        callback = nullptr;
        delta = 0;
        run = false;
    };

    unsigned delta;
    unsigned start;
    bool isPeriodic;
    bool run;
    callback_t callback;
};

class tickTimer
{
    static constexpr unsigned maxTimers = 40;

  public:
    tickTimer();

    int attachPeriodicTimer(callback_t callback, unsigned period) { return addTimer(callback, period, true); }
    int attachOneShotTimer(callback_t callback, unsigned delay) { return addTimer(callback, delay, false); }

    inline void setFrequency(unsigned f);
    inline void trigger(unsigned i)
    {
        channels[i].start = ARM_DWT_CYCCNT;
        channels[i].run = true;
    }

    static inline void tick();

  protected:
    int addTimer(callback_t callback, unsigned t, bool periodic);

    static channelInfo *channels;
};

void tickTimer::tick()
{
    digitalWriteFast(4, HIGH);
    for (unsigned i = 0; i < maxTimers; i++)
    {
        channelInfo *channel = channels + i;

        if (channel->run && (ARM_DWT_CYCCNT - channel->start >= channel->delta))
        {
            if (channel->isPeriodic)
                channel->start = ARM_DWT_CYCCNT;
            else            
                channel->run = false;            

            channel->callback();
        }
    }
    digitalWriteFast(4, LOW);
}

int tickTimer::addTimer(callback_t cb, unsigned delay, bool isPeriodic)
{
    for (unsigned i = 0; i < maxTimers; i++)
    {
        channelInfo *channel = channels + i;
        if (channel->callback == nullptr)
        {
            channel->start = ARM_DWT_CYCCNT;
            channel->isPeriodic = isPeriodic;
            channel->callback = cb;
            channel->delta = delay;
            channel->run = true;

            return i;
        }
    }
    return -1;
}

tickTimer::tickTimer()
{
    ARM_DEMCR |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
}

void tickTimer::setFrequency(unsigned f)
{
}

channelInfo *tickTimer::channels = new channelInfo[tickTimer::maxTimers + 1];

#endif
