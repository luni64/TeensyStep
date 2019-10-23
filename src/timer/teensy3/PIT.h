#pragma once

#include "../../ErrorHandler.h"
#include "IntervalTimer.h"



namespace TeensyStep
{

    // class IPitHandler
    // {
    //  public:
    //     virtual void pitISR() = 0;
    // };

    class TF_Handler;

    class PIT : ErrorHandler
    {

     public:
        pitErr begin(TF_Handler*);

        inline void end()
        {
            //Serial1.println("pit_end");
            timer.end();
            channel = nullptr;
        }
        inline void start() const
        {
            ep(channel, -1);
            channel->TFLG = 1;
            channel->TCTRL = 0;
            channel->TCTRL = 3;
        }
        inline void stop() const
        {
            //Serial1.println("pit_stop");
            ep(channel, -2);
            channel->TCTRL &= ~PIT_TCTRL_TIE;
        }
        inline void enableInterupt() const
        {
            ep(channel, -3);
            channel->TFLG = 1;
            channel->TCTRL |= PIT_TCTRL_TIE;
        }
        inline void setFrequency(uint32_t val) const
        {
            ep(channel, -4);
            channel->LDVAL = F_BUS / val;
        }
        inline void setThisReload(uint32_t ldval) const
        {
            ep(channel, -5);
            channel->TCTRL = 0, channel->TFLG = 1;
            channel->LDVAL = ldval, channel->TCTRL = 3;
        }
        inline void setNextReload(uint32_t ldval) const
        {
            ep(channel, -6);
            channel->LDVAL = ldval;
        }
        inline uint32_t getLDVAL() const
        {
            ep(channel, -7);
            return channel->LDVAL;
        }
        inline uint32_t getCVAL() const
        {
            ep(channel, -8);
            return channel->CVAL;
        }
        inline void clearTIF() const
        {
            ep(channel, -9);
            channel->TFLG = 1;
        }
        inline bool isRunning() const
        {
            return channel != nullptr ? channel->TCTRL & PIT_TCTRL_TIE : false;
        }
		inline bool isAllocated() const
		{
			return channel != nullptr;
		}


     protected:
        KINETISK_PIT_CHANNEL_t* channel = nullptr;
        IntervalTimer timer;
        int setupChannel();

     private:
        inline void ep(void* p, int code) const {if (!p) ErrorHandler::error(errModule::PIT, code);};
        inline pitErr err(pitErr code) const { return (pitErr)error(errModule::PIT, (int)code); }
    };
}