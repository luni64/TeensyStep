#pragma once

#include <cstdint>

namespace TeensyStep
{
    namespace backend
    {
        /// @brief Diagnostics of the Teensy 4 backend, for tests.
        namespace teensy4
        {
            /// @brief Number of calls of a timer interrupt that found nothing to do, since the start of the
            /// program. A command that stops a motor at once can cause one, if the interrupt of the motor was
            /// waiting at that moment. Moves that run and end by themselves cause none.
            uint32_t interruptsWithoutWork();
        }
    }
}
