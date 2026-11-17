#if defined(ARDUINO)

#include "ErrorHandler.h"
#include "Arduino.h"
#include "Stepper.h"

namespace TeensyStep
{
    namespace
    {
        Stream* handlerStream = nullptr;

        void handle(errorCode code)
        {
            if (!handlerStream) return;
            ErrorHandler handler(*handlerStream);
            handler(code);
        }
    }

    ErrorHandler::ErrorHandler(Stream& s) : stream(&s)
    {
        pinMode(LED_BUILTIN, OUTPUT);
    }

    void ErrorHandler::operator()(errorCode code) const
    {
        int number = static_cast<int>(code);
        if (isWarning(code))
        {
            stream->printf("W%d: %s\n", number, describe(code));
            return;
        }
        if (!isError(code)) return;

        emergencyStopAll(); // a sketch that is halted cannot stop its motors any more
        stream->printf("E-%d: %s\n", number, describe(code));
        stream->flush();
        for (;;) // blink; without delay(), which does not work if the error came up in an interrupt
        {
            digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
            for (int i = 0; i < 50; i++) delayMicroseconds(1000);
        }
    }

    void attachErrFunc(const ErrorHandler& handler)
    {
        handlerStream = &handler.output();
        attachErrFunc(handle);
    }
}

#endif
