#pragma once

#include "../../ErrorHandler.h"
#include "IntervalTimer.h"

namespace TeensyStep
{
    class TF_Handler;

    class PIT : ErrorHandler
    {

     public:
        pitErr begin(TF_Handler*);

        inline void end();
        inline void start() const;
        inline void stop() const;
        inline void enableInterupt() const;
        inline void setFrequency(uint32_t val) const;
        inline void setThisReload(uint32_t ldval) const;
        inline void setNextReload(uint32_t ldval) const;
        inline uint32_t getLDVAL() const;
        inline uint32_t getCVAL() const;
        inline void clearTIF() const;
        inline bool isRunning() const;
        inline bool isAllocated() const;

     protected:
        KINETISK_PIT_CHANNEL_t* channel = nullptr;
        IntervalTimer timer;
        int setupChannel();

        inline pitErr err(pitErr code) const { return (pitErr)error(errModule::PIT, (int)code); }
    };


    // Inline implementation ======================================================================

    void PIT::end()
    {
        timer.end();
        channel = nullptr;
    }

    void PIT::start() const
    {
        if (channel == nullptr) err(pitErr::notAllocated);

        channel->TFLG = 1;
        channel->TCTRL = 0;
        channel->TCTRL = 3;
    }

    void PIT::stop() const
    {
        if (channel == nullptr) err(pitErr::notAllocated);
        channel->TCTRL &= ~PIT_TCTRL_TIE;
    }

    void PIT::enableInterupt() const
    {
        if (channel == nullptr) err(pitErr::notAllocated);
        channel->TFLG = 1;
        channel->TCTRL |= PIT_TCTRL_TIE;
    }

    void PIT::setFrequency(uint32_t val) const
    {
        if (channel == nullptr) err(pitErr::notAllocated);
        channel->LDVAL = F_BUS / val;
    }

    void PIT::setThisReload(uint32_t ldval) const
    {
        if (channel == nullptr) err(pitErr::notAllocated);
        channel->TCTRL = 0;
        channel->TFLG = 1;
        channel->LDVAL = ldval;
        channel->TCTRL = 3;
    }

    void PIT::setNextReload(uint32_t ldval) const
    {
        if (channel == nullptr) err(pitErr::notAllocated);
        channel->LDVAL = ldval;
    }

    uint32_t PIT::getLDVAL() const
    {
        if (channel == nullptr) err(pitErr::notAllocated);
        return channel->LDVAL;
    }

    uint32_t PIT::getCVAL() const
    {
        if (channel == nullptr) err(pitErr::notAllocated);
        return channel->CVAL;
    }

    void PIT::clearTIF() const
    {
        if (channel == nullptr) err(pitErr::notAllocated);
        channel->TFLG = 1;
    }

    bool PIT::isRunning() const
    {
        return isAllocated();// && true;// (channel->TCTRL & PIT_TCTRL_TIE);
    }
    
    bool PIT::isAllocated() const
    {
        return channel != nullptr;
    }
}