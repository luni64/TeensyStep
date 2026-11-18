#pragma once

#include <cstdint>

namespace TeensyStep
{
    namespace backend
    {
        /// @brief Diagnostics of the Teensy 3.5 / 3.6 backend, for tests.
        namespace teensy3
        {
            /// @brief Number of calls of a timer interrupt that found nothing to do, since the start of the
            /// program. A command that stops a motor at once can cause one, if the interrupt of the motor was
            /// waiting at that moment. Moves that run and end by themselves cause none.
            uint32_t interruptsWithoutWork();
        }
    }
}
