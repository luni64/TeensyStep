#include "PIT.h"
 

namespace
{
    IPitHandler* pitHandler[4];

    template<int n>
    void dispatchFunc()
    {      
        pitHandler[n]->pitISR();     
    }

    constexpr void(*dispatcher[])(void) =
    {
        dispatchFunc<0>,
        dispatchFunc<1>,
        dispatchFunc<2>,
        dispatchFunc<3>
    };

    void dummyISR(void) {}
}


bool PIT::begin(IPitHandler* handler)
{
    if (!timer.begin(dummyISR, 1E6)) return false;  // try to reserve a timer

    setupChannel();                                 // find pit channel of reserved timer
    const int channelNr = channel - KINETISK_PIT_CHANNELS;
    pitHandler[channelNr] = handler;                // store handler
	timer.priority(32);
    timer.begin(dispatcher[channelNr], 1E6);        // attach an ISR which will call the stored handler
													// don't clear TEN, we want to keep the IntervalTimer reserved
    disableInterupt();     		

    return true;
}

void PIT::setupChannel()
{
    IRQ_NUMBER_t number = (IRQ_NUMBER_t)timer;
    switch (number)
    {
    case IRQ_PIT_CH0:
        channel = KINETISK_PIT_CHANNELS + 0;
        break;
    case IRQ_PIT_CH1:
        channel = KINETISK_PIT_CHANNELS + 1;
        break;
    case IRQ_PIT_CH2:
        channel = KINETISK_PIT_CHANNELS + 2;
        break;
    case IRQ_PIT_CH3:
        channel = KINETISK_PIT_CHANNELS + 3;
        break;
    default:
        channel = nullptr;
        break;
    }
}