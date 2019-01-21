#include "TickTimer.h"

void std::__throw_bad_function_call()
{
    while(1);
}

TimerBase* TimerControl::firstTimer = nullptr;
TimerBase* TimerControl::lastTimer = nullptr;