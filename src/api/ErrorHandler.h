#pragma once

// The ready-made error function: attachErrFunc(ErrorHandler(Serial));

#if defined(ARDUINO)

#include "../core/errorCode.h"
#include "Stream.h"

namespace TeensyStep
{
    /// @brief Ready-made error function for attachErrFunc(): `attachErrFunc(ErrorHandler(Serial));`
    /// @details Warnings are printed as "W-<code>: <text>" and the sketch continues. Errors are printed as
    /// "E-<code>: <text>"; then the handler stops all motors at once and blinks LED_BUILTIN in an endless
    /// loop. The handler uses the same pattern as the one of TeensyTimerTool.
    class ErrorHandler
    {
     public:
        /// @brief Creates the handler; sets LED_BUILTIN to output.
        /// @param stream stream for the messages, e.g. Serial
        ErrorHandler(Stream& stream);

        /// @brief Prints the message for `code`; for errors it stops all motors and does not return.
        void operator()(errorCode code) const;

        Stream& output() const { return *stream; }

     protected:
        Stream* stream;
    };

    /// @brief Attaches the ready-made handler. The stream has to stay valid.
    void attachErrFunc(const ErrorHandler& handler);
}

#endif
