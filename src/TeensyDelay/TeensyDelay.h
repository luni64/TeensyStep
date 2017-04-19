#pragma once
#include "config.h"

class IDelayHandler
{
public:
    virtual void delayISR(unsigned channel) = 0;
};

namespace TeensyDelay
{
    extern void begin(void);

#ifdef USE_CLASS_CALLBACK
    extern unsigned addDelayChannel(IDelayHandler* h, const int nr = -1);
#else
    extern void addDelayChannel(void(*_callback)(void), const int nr = 0);
#endif // USE_CLASS_CALLBACK


    inline void trigger(const float delay, const int channel = 0)
    {
        timer->SC = 0;                                                 // Remove clock to immediately write new channel value
        timer->CH[channel].CV = timer->CNT + microsToReload(delay);    // Update channel value
        timer->SC = FTM_SC_CLKS(0b01) | FTM_SC_PS(prescale);           // Reattach clock       

        if (isFTM) {                                                   // important to clear channel flag (in case we had a channel event since last trigger)
            timer->CH[channel].SC &= ~FTM_CSC_CHF;                     // enable channel interrupt. Can not combine both commands since clearing 		 
        }
        else {                                                         // the channel flag requires a read modify write cycle            
            timer->CH[channel].SC |= FTM_CSC_CHF;                      // FTM and TPM timers differ in resetting the interrupt flag
        }                                                              // isFTM is a compile time constant -> compiler completely optimizes the not valid case away
        timer->CH[channel].SC = FTM_CSC_MSA | FTM_CSC_CHIE;
    }
}
