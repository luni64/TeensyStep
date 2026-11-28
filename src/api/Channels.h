#pragma once

// Internal: the channels of the library. A channel carries one running move, of a single motor or of a group:
// its ramp, its motors, and one timer of the backend. Channel i of the library is channel i of the backend.

#include "../backend/Backend.h"
#include "../core/Axis.h"
#include "../core/Group.h"

namespace TeensyStep
{
    class Stepper;
    class StepperGroup;
    namespace advanced
    {
        class GroupPath;
    }

    namespace detail
    {
        struct Channel
        {
            bool used    = false;
            bool isGroup = false;
            bool ending  = false; // the move is over; the channel is kept until the drivers are switched off
            uint8_t count = 0;    // motors
            Stepper* motors[backend::maxMotors] = {};
            backend::MotorPins pins[backend::maxMotors];
            float pulseWidth = 5, dirSetup = 5;

            core::Axis axis;   // the ramp of a single motor
            core::Group group; // the ramp of a group

            void (*arrival)() = nullptr; // group: called when the targets are reached
            void (*arrivalGroup)(StepperGroup&) = nullptr; // or this one, with the group object that gave the move ...
            StepperGroup* groupObject = nullptr;           // ... as long as that object exists
            advanced::GroupPath* pathOwner = nullptr; // group: the path whose moves run in this channel

            // enable signals of the drivers
            float startDelay = 0; // seconds still to wait before the first step, while the drivers switch on
            bool starting    = false; // startDelay > 0 (kept apart, so that the interrupt needs no floating point to see it)
            float offDelay   = 0; // seconds the drivers stay on after the move
            float offLeft    = 0; // remaining seconds of that time

            // limits of the members of a group, as the version 2 controllers use them
            float speedFactor   = 1;     // maximum speeds are scaled by this factor
            bool unlimitedSpeed = false; // the maximum speed of a member is no limit (rotation with speed factors above 1)
        };

        /// The functions that work on the channels. A struct, so that Stepper can give it access.
        struct Engine
        {
            static Channel channels[backend::maxChannels];

            static errorCode report(errorCode code);

            // Takes a free channel for the motors, sets it up and opens it in the backend.
            // Returns its index, or -1 with the reason in `code`.
            static int setup(Stepper* const* motors, unsigned count, bool isGroup, float speedFactor, bool unlimitedSpeed, errorCode& code);

            static void release(unsigned ch, const int32_t* notExecuted); // gives the positions back to the motors and frees the channel
            static void endNow(unsigned ch);                              // stops the backend and releases the channel
            static void emergencyStop(unsigned ch);                       // like endNow, but the drivers stay on for their off-delay
            static core::MotorLimits limitsOf(const Stepper& s, const Channel& c);
            static bool isMoving(int channel);
            static void switchDriver(Stepper& s, bool on); // the enable signal of a motor that joins or leaves a channel

            static bool fetch(unsigned ch, backend::Event& event);
            static void arrived(unsigned ch);
            static void finished(unsigned ch);
        };

        struct Lock // keeps the backend from calling back while a channel is changed
        {
            Lock() { backend::lock(); }
            ~Lock() { backend::unlock(); }
        };
    }
}
