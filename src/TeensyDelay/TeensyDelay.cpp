#include "TeensyDelay.h" 
#include "config.h"

namespace TeensyDelay
{
#ifdef USE_CLASS_CALLBACK
	IDelayHandler* callbacks[maxChannel];
#else
	void(*callbacks[maxChannel])(void);
#endif

	unsigned lastChannel = 0;

	void begin()
	{
		if (USE_TIMER == TIMER_TPM1) {                // enable clocks for tpm timers (ftm clocks are enabled by teensyduino)        
			SIM_SCGC2 |= SIM_SCGC2_TPM1;
			SIM_SOPT2 |= SIM_SOPT2_TPMSRC(2);
		}
		else if (USE_TIMER == TIMER_TPM2) {
			SIM_SCGC2 |= SIM_SCGC2_TPM2;
			SIM_SOPT2 |= SIM_SOPT2_TPMSRC(2);
		}

		//Default Mode for  FTM is (nearly) TPM compatibile 
		timer->SC = FTM_SC_CLKS(0b00);              // Disable clock		        
		timer->MOD = 0xFFFF;                        // SEt full counter range

		for (unsigned i = 0; i < maxChannel; i++) {  // turn off all channels which were enabled by teensyduino for PWM generation                                                   
			if (isFTM) {                            // compiletime constant, compiler optimizes conditional and not valid branch completely away           
				timer->CH[i].SC &= ~FTM_CSC_CHF;    // FTM requires to clear flag by setting bit to 0    
			}
			else {
				timer->CH[i].SC |= FTM_CSC_CHF;     // TPM requires to clear flag by setting bit to 1
			}
			timer->CH[i].SC &= ~FTM_CSC_CHIE;       // Disable channel interupt
            timer->CH[i].SC = FTM_CSC_MSA;
			//timer->CH[i].SC = 0;                    // disable channel
		}
		timer->SC = FTM_SC_CLKS(0b01) | FTM_SC_PS(prescale);  // Start clock
		NVIC_ENABLE_IRQ(irq);                        // Enable interrupt request for selected timer
	}

#ifdef USE_CLASS_CALLBACK
	unsigned addDelayChannel(IDelayHandler* handler, const int channel)
	{
		unsigned ch = channel < 0 ? lastChannel++ : channel;
		callbacks[ch] = handler;               //Just store the callback function, the rest is done in Trigger function
		return ch;
	}
#else
	void addDelayChannel(void(*callback)(void), const int channel = 0)
	{
		callbacks[channel] = callback;               //Just store the callback function, the rest is done in Trigger function
	}

#endif // USE_CLASS_CALLBACK
}

//-------------------------------------------------------------------------------------------
// Interupt service routine of the timer selected in config.h. 
// The code doesn't touch the other FTM/TPM ISRs so they can still be used for other purposes
//
// Unfortunately we can not inline the ISR because inlinig will generate a "weak" function. 
// Since the original ISR (dummy_isr) is also defined weak the linker
// is allowed to choose any of them. In this case it desided to use dummy_isr :-(
// Using a "strong" (not inlined) function overrides the week dummy_isr
//--------------------------------------------------------------------------------------------

using namespace TeensyDelay;

#if USE_TIMER == TIMER_FTM0
void ftm0_isr(void)
#elif USE_TIMER == TIMER_FTM1
void ftm1_isr(void)
#elif USE_TIMER == TIMER_FTM2
void ftm2_isr(void)
#elif USE_TIMER == TIMER_FTM3
void ftm3_isr(void)
#elif USE_TIMER == TIMER_TPM1
void tpm1_isr(void)
#elif USE_TIMER == TIMER_TPM2
void tpm2_isr(void)
#endif
{
    //digitalWriteFast(14, HIGH);
    uint32_t status = timer->STATUS & 0x0F;   // STATUS collects all channel event flags (bit0 = ch0, bit1 = ch1....) 

    unsigned i = 0;
    while (status > 0) {
        if (status & 0x01) {
            if (isFTM) {                           // isFTM is a compiletime constant, compiler optimizes conditional and not valid branch completely away           
                timer->CH[i].SC &= ~FTM_CSC_CHF;   // reset channel and interrupt enable (we only want one shot per trigger)	
            }
            else {
                timer->CH[i].SC |= FTM_CSC_CHF;    // TPM needs inverse setting of the flags                
            }

            if (timer->CH[i].SC & FTM_CSC_CHIE)    // Channel flags will be set each time the counter overflows the channel value. 
            {									   // In case that the interupt was triggered by another channel we need to prevent
                timer->CH[i].SC &= ~FTM_CSC_CHIE;  // the callback if this channel was not triggerd (CHIE of this channel not set)
#ifdef  USE_CLASS_CALLBACK
                callbacks[i]->delayISR(i);
#else
                callbacks[i]();				       // invoke callback function for the channel								                     
#endif //  USE_CLASS_CALLBACK
            }
        }
        i++;
        status >>= 1;
    }
    //digitalWriteFast(14, LOW);
}