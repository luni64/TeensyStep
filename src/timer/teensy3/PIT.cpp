#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
#include "PIT.h"
#include "../TF_Handler.h"

namespace TeensyStep
{

    namespace
    {
        TF_Handler* Handler[4];

        template <int n>
        void dispatchFunc()
        {
            Handler[n]->stepTimerISR();
        }

        constexpr void (*dispatcher[])(void) =
            {
                dispatchFunc<0>,
                dispatchFunc<1>,
                dispatchFunc<2>,
                dispatchFunc<3>};

        void dummyISR(void) {}
    }

    pitErr PIT::begin(TF_Handler* handler)
    {
        if (timer.begin(dummyISR, 1E6)) // try to reserve a timer
        {
            const int channelNr = setupChannel();    // find pit channel of reserved timer            
            Handler[channelNr] = handler;            // store handler
            timer.priority(32);
            timer.begin(dispatcher[channelNr], 1E6); // attach an ISR which will call the stored handler
            stop();                                  // stop doesn't clear TEN, we want to keep the IntervalTimer reserved
            return pitErr::OK;
        }
        return err(pitErr::outOfTimers);
    }

    int PIT::setupChannel()
    {
        IRQ_NUMBER_t number = (IRQ_NUMBER_t)timer;
        switch (number)
        {
            case IRQ_PIT_CH0:
                channel = KINETISK_PIT_CHANNELS + 0;
                return 0;                
            case IRQ_PIT_CH1:
                channel = KINETISK_PIT_CHANNELS + 1;
                return 1;
            case IRQ_PIT_CH2:
                channel = KINETISK_PIT_CHANNELS + 2;
                return 2;
            case IRQ_PIT_CH3:
                channel = KINETISK_PIT_CHANNELS + 3;
                return 3;
            default:
                channel = nullptr;
                err(pitErr::argErr);
                return -1;                
        }
    }
}
#endif
