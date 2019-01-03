#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

#include "TeensyStepFTM.h"
#include "config.h"

namespace TeensyStepFTM
{

namespace // private
{
	bool isConfigured = false;
	IDelayHandler *callbacks[maxChannel];
} 

void begin()
{
	if (!isConfigured)
	{
		if (USE_TIMER == TIMER_TPM1)
		{ 										// enable clocks for tpm timers (ftm clocks are enabled by teensyduino)
			SIM_SCGC2 |= SIM_SCGC2_TPM1;
			SIM_SOPT2 |= SIM_SOPT2_TPMSRC(2);
		}
		else if (USE_TIMER == TIMER_TPM2)
		{
			SIM_SCGC2 |= SIM_SCGC2_TPM2;
			SIM_SOPT2 |= SIM_SOPT2_TPMSRC(2);
		}
													// Default Mode for  FTM is (nearly) TPM compatibile
		timer->SC = FTM_SC_CLKS(0b00); 				// Disable clock
		timer->MOD = 0xFFFF;		   				// Set full counter range

		for (unsigned i = 0; i < maxChannel; i++)
		{ 											// turn off all channels which were enabled by teensyduino for PWM generation
			if (isFTM)
			{									 	// compiletime constant, compiler optimizes conditional and not valid branch completely away
				timer->CH[i].SC &= ~FTM_CSC_CHF; 	// FTM requires to clear flag by setting bit to 0
			}
			else
			{
				timer->CH[i].SC |= FTM_CSC_CHF; 	// TPM requires to clear flag by setting bit to 1
			}
			timer->CH[i].SC &= ~FTM_CSC_CHIE; 		// Disable channel interupt
			timer->CH[i].SC = FTM_CSC_MSA;
			//timer->CH[i].SC = 0;                   // disable channel
		}
		timer->SC = FTM_SC_CLKS(0b01) | FTM_SC_PS(prescale); // Start clock
		NVIC_ENABLE_IRQ(irq);						 // Enable interrupt request for selected timer

		isConfigured = true;
	}
}

unsigned addDelayChannel(IDelayHandler *handler)
{
	for (unsigned ch = 0; ch < maxChannel; ch++)
	{
		if (callbacks[ch] == 0)
		{
			callbacks[ch] = handler; //Just store the callback function, the rest is done in Trigger function
			return ch;
		}
	}
	return maxChannel;
}

void removeDelayChannel(unsigned chNr)
{
	if (chNr < maxChannel)
		callbacks[chNr] = nullptr;
}
} // namespace TeensyDelay2

//-------------------------------------------------------------------------------------------
// Interupt service routine of the timer selected in config.h.
// The code doesn't touch the other FTM/TPM ISRs so they can still be used for other purposes
//
// Unfortunately we can not inline the ISR because inlinig will generate a "weak" function?.
// Since the original ISR (dummy_isr) is also defined weak the linker
// is allowed to choose any of them. In this case it desided to use dummy_isr :-(
// Using a "strong" (not inlined) function overrides the week dummy_isr
//--------------------------------------------------------------------------------------------

using namespace TeensyStepFTM;

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
	for (unsigned i = 0; i < maxChannel; i++)
	{
		if ((timer->CH[i].SC & (FTM_CSC_CHIE | FTM_CSC_CHF)) == (FTM_CSC_CHIE | FTM_CSC_CHF)) // only handle if channel is active (CHIE set) and overflowed (CHF set)
		{
			timer->CH[i].SC &= ~FTM_CSC_CHIE; // We want one shot only. (Make sure to reset the CHF flag before re-activating interrupt in trigger function)
			callbacks[i]->delayISR(i);		  // invoke callback function for the channel
		}
	}
}
#endif