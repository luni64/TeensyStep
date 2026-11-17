#pragma once

// Simulated backend for the PC: no hardware and no real time. It keeps a clock that is advanced by the test,
// executes the events of the channels in the order of their times and records every change of a pin, like a
// logic analyzer. It runs in two ways, so that the library is tested against both kinds of backends:
// one event per "interrupt" (the next event is fetched when the current one is executed), or buffered (events
// are fetched ahead of the clock by a look-ahead time).

#if !defined(ARDUINO)

#include "../Backend.h"
#include <cstdint>
#include <vector>

namespace TeensyStep
{
    namespace backend
    {
        namespace host
        {
            struct Edge
            {
                double t; ///< seconds
                uint8_t pin;
                bool level;
            };

            /// @brief What a recording shows for one motor.
            struct Pulses
            {
                long count  = 0; ///< step pulses
                long up     = 0; ///< pulses with DIR at "count up"
                long down   = 0;
                long net    = 0; ///< up - down: the distance the motor went
                double first = 0, last = 0;          ///< times of the first and the last pulse
                double minWidth = 0, maxWidth = 0;   ///< pulse widths, seconds
                double minPeriod = 0, maxPeriod = 0; ///< times between two pulses
                long dirTooLate = 0;                 ///< pulses that came sooner after a change of DIR than the setup time
                std::vector<double> times;           ///< time of every pulse
            };

            void reset();                       ///< clock at 0, all pins and channels forgotten, one event per interrupt, no limit of timers
            void setLookahead(double seconds);  ///< 0: one event per interrupt. > 0: buffered, events are fetched this far ahead of the clock
            void setTimers(unsigned count);     ///< number of timers of the simulated board
            void setMaxDelay(float seconds);    ///< value returned by backend::maxDelay(), default 0.001

            void run(double seconds);                    ///< advances the clock and executes what is due
            bool runUntilIdle(double maxSeconds = 1e7); ///< runs until no channel is open; false if that takes longer than maxSeconds
            double now();

            const std::vector<Edge>& edges();   ///< every change of a pin since reset(), in the order of time
            bool level(uint8_t pin);            ///< current level of a pin
            unsigned openChannels();            ///< channels that hold a timer
            long fetchesWhileLocked();          ///< must stay 0: fetch() or arrived() called while the library held the lock
            Pulses pulses(const MotorPins& motor, double since = 0, double dirSetup = 0); ///< the pulses of a motor from time `since` on
        }
    }
}

#endif
