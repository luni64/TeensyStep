#pragma once

#include "../core/errorCode.h"
#include <cstdint>

// Number of moves that can run at the same time (single motors and groups together). Each one needs a
// channel, and each channel a hardware timer. The channels are allocated statically.
#ifndef TEENSYSTEP_MAX_CHANNELS
#if defined(__MKL26Z64__) // Teensy LC: 8 kB of RAM
#define TEENSYSTEP_MAX_CHANNELS 2
#elif defined(__MK20DX128__) || defined(__MK20DX256__) // Teensy 3.0, 3.1, 3.2
#define TEENSYSTEP_MAX_CHANNELS 4
#else
#define TEENSYSTEP_MAX_CHANNELS 16
#endif
#endif

namespace TeensyStep
{
    /// @brief The interface between the library and the hardware.
    ///
    /// @details A backend makes the step pulses on one processor family. Exactly one backend is compiled
    /// into a program, selected by the platform; there are no virtual functions.
    ///
    /// The unit of work is a channel: one motor, or one group of motors that move synchronized. A channel
    /// has one timer. The library opens a channel with the pins of its motors and starts it. From then on
    /// the backend asks for one event after the other with fetch(): wait for the delay, then make the steps
    /// named in the event. When fetch() returns false the move is over; the backend stops the timer and
    /// the channel is closed.
    ///
    /// Functions the backend provides:
    ///
    ///     maxDelay()   longest delay it can wait in one event (range of its timer)
    ///     initMotor()  set the pins of a motor to outputs at their idle levels
    ///     setPin()     set another output pin (enable signals of the drivers)
    ///     open()       take a timer for a channel and remember its pins; reports if no timer is free
    ///     start()      begin to fetch and execute events for the channel
    ///     halt()       stop the channel at once and say which fetched steps were not made
    ///     lock()       keep fetch() and arrived() out while the library changes a channel
    ///     unlock()
    ///     idle()       called repeatedly while a blocking command waits
    ///     setInterruptPriority()  priority of the interrupts in which the steps are made
    ///
    /// Functions the library provides, called by the backend:
    ///
    ///     fetch()      the next event of a channel
    ///     arrived()    a target was reached
    ///     finished()   the channel has ended by itself and is closed
    namespace backend
    {
        constexpr unsigned maxChannels = TEENSYSTEP_MAX_CHANNELS;
        constexpr unsigned maxMotors   = 16;  ///< motors of one channel
        constexpr uint8_t noPin        = 255; ///< for a signal that is not connected

        /// Priority of the step interrupts unless setInterruptPriority() is called (lower number: more
        /// urgent). Above the default of the Teensy core (128), so that other interrupts do not delay the
        /// steps, and one level below its millisecond interrupt (32): whatever holds up a step interrupt,
        /// for example an arrival function that takes long, does not stop millis().
        constexpr uint8_t defaultInterruptPriority = 48;

        /// @brief The pins of one motor and their polarities.
        struct MotorPins
        {
            uint8_t step         = noPin;
            uint8_t dir          = noPin;
            bool stepActiveHigh  = true; ///< true: the step pulse is a high pulse
            bool dirHighCountsUp = true; ///< true: DIR is high while the position counts up
        };

        /// @brief What a channel drives.
        struct ChannelSetup
        {
            const MotorPins* motors = nullptr; ///< `count` motors; motor i belongs to bit i of the event masks
            unsigned count          = 0;
            float pulseWidth        = 5; ///< width of the step pulses, microseconds
            float dirSetup          = 5; ///< time DIR has to be stable before a step, microseconds
        };

        /// @brief One event of a channel: wait for `delay`, then make the steps.
        struct Event
        {
            float delay;       ///< seconds since the previous event of the channel (since start(), for the first one); at most maxDelay()
            uint16_t stepMask; ///< bit i: motor i makes a step. 0: no step, wait only
            uint16_t dirMask;  ///< bit i: motor i counts up (1) or down (0) when it steps
            bool arrived;      ///< the event reaches a target: the backend calls arrived(), see there
        };

        //----------------------------------------------------------------------------------------------------
        // provided by the backend

        /// @brief Longest delay, in seconds, that the backend can wait in one event.
        float maxDelay();

        /// @brief Sets the step and direction pins of a motor to outputs at their idle levels.
        void initMotor(const MotorPins& pins);

        /// @brief Makes the pin an output with this level. Used for the enable signals of the drivers.
        void setPin(uint8_t pin, bool level);

        /// @brief Takes a timer for the channel and remembers its motors. The setup stays valid until the
        /// channel is closed.
        /// @return OK, or noTimer if no timer is free
        errorCode open(unsigned channel, const ChannelSetup& setup);

        /// @brief Begins to execute the channel: the backend calls fetch() for the first event now or
        /// shortly after, and for each further event when it needs it. No effect if the channel runs already;
        /// but if fetch() has returned false and the channel has not finished yet, the backend fetches again.
        ///
        /// A backend that executes one event per interrupt fetches the next event after it has made the
        /// steps of the current one. A buffered backend fetches ahead. When fetch() returns false, the
        /// backend executes what it has fetched, releases the timer and calls finished().
        ///
        /// Before a step, DIR of the stepping motors has the level of the event for at least `dirSetup`.
        void start(unsigned channel);

        /// @brief Stops the channel at once: no further step, no further fetch(), the channel is closed.
        /// A step pulse that has begun is completed.
        /// @param notExecuted one value per motor: the steps that were fetched but not made, counted in
        ///        the direction of the position (+ up, - down). The library corrects the positions by them.
        void halt(unsigned channel, int32_t* notExecuted);

        /// @brief Keeps fetch() and arrived() from running until unlock(). Calls can be nested. The library
        /// holds the lock only for a few instructions, while it changes the state of a channel.
        void lock();
        void unlock();

        /// @brief Called repeatedly by a blocking command while it waits for a move to end.
        void idle();

        /// @brief Sets the priority of the interrupts in which the steps are made, as for
        /// NVIC_SET_PRIORITY: 0 (most urgent) to 255, of which the processor uses the upper four bits.
        /// Takes effect at once, also for moves that are under way. No effect on a backend without
        /// interrupts of its own.
        void setInterruptPriority(uint8_t priority);

        //----------------------------------------------------------------------------------------------------
        // provided by the library, called by the backend (from its interrupt)

        /// @brief Calculates the next event of the channel.
        /// @return false if the move is over: there is no event, and the channel is closed
        bool fetch(unsigned channel, Event& event);

        /// @brief To be called for an event with the `arrived` flag, before the next fetch() of the
        /// channel: after the steps of the event were made (one event per interrupt), or when the event was
        /// queued (buffered backend). The library calls the arrival function of the sketch, which may give
        /// the next command.
        void arrived(unsigned channel);

        /// @brief To be called when the channel has ended by itself: fetch() has returned false and the
        /// last fetched event was executed. The backend has released the timer; the channel is closed.
        /// (Not called after halt().)
        void finished(unsigned channel);
    }
}
