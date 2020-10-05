#include "TickTimer.h"

void std::__throw_bad_function_call()
{
    while(1);
}

float PeriodicTimer::minFrequency = (float)F_CPU / std::numeric_limits<uint32_t>::max();
TimerBase* TimerControl::firstTimer = nullptr;
TimerBase* TimerControl::lastTimer = nullptr;
