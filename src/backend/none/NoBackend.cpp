// Placeholder for boards that have no backend yet (Teensy 3.x, LC): the library compiles and links, and every
// move command reports noTimer.

#if defined(ARDUINO) && !defined(__IMXRT1062__)

#include "../Backend.h"

namespace TeensyStep
{
    namespace backend
    {
        float maxDelay() { return 0.001f; }
        void initMotor(const MotorPins&) {}
        void setPin(uint8_t, bool) {}
        errorCode open(unsigned, const ChannelSetup&) { return errorCode::noTimer; }
        void start(unsigned) {}
        void halt(unsigned, int32_t*) {}
        void lock() {}
        void unlock() {}
        void idle() {}
    }
}

#endif
