#pragma once

#include "../../ErrorHandler.h"
#include "IntervalTimer.h"

// class IPitHandler
// {
// public:
// 	virtual void pitISR() = 0;
// };

class TF_Handler;

inline void ep(void* p, int code)
{
    if (p == nullptr) TeensyStep::ErrorHandler::error(99, code);
}

class PIT : TeensyStep::ErrorHandler
{
 public:
    bool begin(TF_Handler*);

    inline void end() { timer.end(); }
    inline void start() const
    {
        ep(channel, 1);
        channel->TFLG = 1;
        channel->TCTRL = 0;
        channel->TCTRL = 3;
    }
    inline void stop() const
    {
        ep(channel, 2);
        channel->TCTRL &= ~PIT_TCTRL_TIE;
    }
    inline void enableInterupt() const
    {
        ep(channel, 3);
        channel->TFLG = 1;
        channel->TCTRL |= PIT_TCTRL_TIE;
    }
    inline void setFrequency(uint32_t val) const
    {
        ep(channel, 4);
        channel->LDVAL = F_BUS / val;
    }
    inline void setThisReload(uint32_t ldval) const
    {
        ep(channel, 5);
        channel->TCTRL = 0, channel->TFLG = 1;
        channel->LDVAL = ldval, channel->TCTRL = 3;
    }
    inline void setNextReload(uint32_t ldval) const
    {
        ep(channel, 6);
        channel->LDVAL = ldval;
    }
    inline uint32_t getLDVAL() const
    {
        ep(channel, 7);
        return channel->LDVAL;
    }
    inline uint32_t getCVAL() const
    {
        ep(channel, 8);
        return channel->CVAL;
    }
    inline void clearTIF() const
    {
        ep(channel, 9);
        channel->TFLG = 1;
    }
    inline bool isRunning() const
    {      
        return channel != nullptr ?  channel->TCTRL & PIT_TCTRL_TIE : false;
    }

    KINETISK_PIT_CHANNEL_t* channel = nullptr;

 protected:
    IntervalTimer timer;
    void setupChannel();
};