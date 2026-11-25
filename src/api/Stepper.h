#pragma once

#include "../backend/Backend.h"
#include "../core/errorCode.h"
#include <cstddef>
#include <cstdint>

namespace TeensyStep
{
    namespace detail
    {
        struct Engine;
    }

    /// @brief Attaches a function that is called for every warning and error of the library. nullptr removes it.
    void attachErrFunc(void (*handler)(errorCode));

    /// @brief Stops all motors at once, without deceleration.
    void emergencyStopAll();

    /// @brief A stepper motor on a driver with STEP and DIR inputs.
    ///
    /// @details The commands are given to the motor. A command with `Async` in its name returns at once and
    /// the motor moves in the background; the command without it returns when the motor has arrived.
    /// A new command can be given at any time and replaces the current one: the motor changes to the new
    /// target or speed without stopping first.
    ///
    /// Several motors move synchronized as a StepperGroup. While a motor moves in a group, it takes no move
    /// commands of its own (they return motorBusy); stop() and emergencyStop() stop the whole group.
    ///
    /// Speeds are in steps/s, accelerations in steps/s², positions in steps.
    class Stepper
    {
     public:
        Stepper(int stepPin, int dirPin);
        ~Stepper(); ///< a moving motor is stopped at once

        Stepper(const Stepper&)            = delete;
        Stepper& operator=(const Stepper&) = delete;

        // Motion settings. Chainable; allowed at any time, also during a move. A value outside the supported
        // range is limited and reported to the error function.
        Stepper& setMaxSpeed(float stepsPerSec);       ///< default 800
        Stepper& setAcceleration(float stepsPerSec2);  ///< default 2500; also sets the deceleration, until setDeceleration() is used
        Stepper& setDeceleration(float stepsPerSec2);
        Stepper& setPullInSpeed(float stepsPerSec);    ///< speed up to which the motor starts and stops without a ramp, default 100

        // Hardware settings. Chainable; to be set while the motor stands still (during a move they are
        // refused and notSupported is reported to the error function).
        Stepper& setPulseWidth(float microseconds);    ///< width of the step pulse, default 5
        Stepper& setStepPinPolarity(int activeLevel);  ///< HIGH (1): positive step pulses (default), LOW (0): negative
        Stepper& setInverseRotation(bool inverse);     ///< true: swaps the direction of the motor
        Stepper& setDirSetupTime(float microseconds);  ///< time between a change of DIR and the next step, default 5

        /// @brief Optional: the enable input of the driver. The library switches the driver on before a
        /// move and off after it. Without this call the library does not touch the driver.
        /// @param activeLevel level that switches the driver on, LOW (0, default) or HIGH (1)
        /// @param onDelayMs   time between switching on and the first step
        /// @param offDelayMs  time the driver stays on after the move; a new move within it starts at once
        Stepper& setEnablePin(int pin, int activeLevel = 0, unsigned onDelayMs = 0, unsigned offDelayMs = 0);

        /// @brief Sets the position counter. Does not move the motor; a target stays where it is.
        void setPositionCounter(int32_t pos);

        // Commands

        /// @brief Move to `pos`. speedAtTarget = 0: stop there. Otherwise: pass `pos` at this speed (the
        /// sign is the direction of passing) and keep running at it.
        /// @return OK, a warning (speedLimited), or an error (noTimer, motorBusy): then the motor is unchanged
        errorCode moveAbsAsync(int32_t pos, float speedAtTarget = 0);
        errorCode moveRelAsync(int32_t delta, float speedAtTarget = 0); ///< like moveAbsAsync, relative to the current position
        errorCode rotateAsync(float stepsPerSec);                       ///< run at this speed (sign: direction); 0 stops
        errorCode stopAsync();                                          ///< decelerate to standstill
        void emergencyStop();                                           ///< stand still at once, without deceleration

        errorCode moveAbs(int32_t pos, float speedAtTarget = 0); ///< returns when the motor stands still at the target (or passes it, with a speed)
        errorCode moveRel(int32_t delta, float speedAtTarget = 0);
        errorCode stop();                                        ///< returns when the motor stands still

        // Targets for the moves of a group: StepperGroup::move() moves every member to its target.
        void setTargetAbs(int32_t pos);
        void setTargetRel(int32_t delta); ///< relative to the current position

        // State
        int32_t getPosition() const;
        float getSpeed() const;  ///< current speed, steps/s; 0 when the motor stands still
        bool isMoving() const;

        /// @brief Sets a function that is called when a target is reached. It may give the next command.
        /// It is called from an interrupt: keep it short. nullptr removes it.
        ///
        /// The function can have the motor as its parameter, so that one function serves several motors:
        ///
        ///     void arrived(Stepper& motor) { ... }
        ///     x.onArrival(arrived);
        ///     y.onArrival(arrived);
        void onArrival(void (*callback)());
        void onArrival(void (*callback)(Stepper& motor)); ///< called with the motor that has arrived
        void onArrival(std::nullptr_t);                   ///< removes the function

        /// @deprecated The old name of setPositionCounter().
        [[deprecated("use setPositionCounter()")]] void setPosition(int32_t pos) { setPositionCounter(pos); }

        /// @deprecated Version 2: separate pull-in and pull-out speeds. The lower one is used for both.
        [[deprecated("use setPullInSpeed()")]] Stepper& setPullInOutSpeed(float pullInSpeed, float pullOutSpeed);

     private:
        enum class Command : uint8_t {
            moveAbs,
            moveRel,
            rotate,
            stop,
        };
        errorCode command(Command cmd, int32_t pos, float speed);
        void waitForArrival(uint32_t arrivalsBefore, float speedAtTarget);
        void applyLimits();
        bool hardwareSetting();

        backend::MotorPins pins;
        float vMax = 800, acc = 2500, dec = 2500, pullIn = 100;
        bool separateDeceleration = false;
        bool reversed             = false; // version 2: a negative maximum speed, the direction of RotateControl
        float pulseWidth = 5, dirSetup = 5;

        uint8_t enablePin     = backend::noPin;
        bool enableActiveHigh = false;
        float onDelay = 0, offDelay = 0; // seconds

        int32_t pos    = 0;     // position while the motor has no channel
        int32_t target = 0;     // target for group moves
        bool hasTarget = false; // false: the target is the current position
        int8_t channel = -1;    // channel of the running move, -1: none
        uint8_t member = 0;     // index of the motor in its channel
        void (*arrival)()              = nullptr; // arrival function without a parameter, or
        void (*arrivalMotor)(Stepper&) = nullptr; // the one that gets the motor; at most one of them is set
        void callArrival();
        volatile uint32_t arrivals = 0; // counts the reached targets, for the blocking commands

        friend struct detail::Engine;
        friend class StepperGroup;
        friend class StepControl;
        friend class RotateControl;
    };
}
