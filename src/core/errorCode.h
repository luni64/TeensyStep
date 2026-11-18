#pragma once

namespace TeensyStep
{
    /// @brief Result of the commands: OK (0), warnings (negative: done with a restriction),
    /// errors (positive: not done, the motor is unchanged).
    /// @details The pattern is the one of TeensyTimerTool. The values are in a range of their own
    /// (errors 1000 .. 1999, warnings -1000 .. -1999), so that a printed code identifies the library.
    enum class errorCode {
        OK = 0, ///< success

        // Warnings (negative): the command was executed with a restriction
        speedLimited        = -1000, ///< warning: speed outside the supported range, the limit is used
        accelerationLimited = -1001, ///< warning: acceleration outside the supported range, the limit is used
        targetSpeedNotReached = -1002, ///< warning: the way is too short for the speed at the target; a group arrives as close to it as possible

        // Errors (positive): the command was not executed
        invalidArgument = 1000, ///< error: invalid argument, e.g. a group move with the wrong number of targets
        motorBusy       = 1001, ///< error: the motor is moving in another group
        noTimer         = 1002, ///< error: no free hardware timer for this move
        notSupported    = 1003, ///< error: not possible in this state, e.g. a new target for a moving group

        internal = 1999, ///< error: inconsistent state inside the library
    };

    /// @brief True for a warning: the command was executed with a restriction.
    constexpr bool isWarning(errorCode c) { return static_cast<int>(c) < 0; }

    /// @brief True for an error: the command was not executed.
    constexpr bool isError(errorCode c) { return static_cast<int>(c) > 0; }

    /// @brief A short text for a code, as the ErrorHandler prints it.
    inline const char* describe(errorCode c)
    {
        switch (c)
        {
            case errorCode::OK: return "OK";
            case errorCode::speedLimited: return "Speed outside the supported range or above the maximum speed. The limit is used";
            case errorCode::accelerationLimited: return "Acceleration outside the supported range. The limit is used";
            case errorCode::targetSpeedNotReached: return "The way is too short for the speed at the target";
            case errorCode::invalidArgument: return "Invalid argument, e.g. wrong number of targets for a group";
            case errorCode::motorBusy: return "The motor is moving in another group";
            case errorCode::noTimer: return "No free hardware timer for this move";
            case errorCode::notSupported: return "Not possible in this state, e.g. new targets for a moving group";
            case errorCode::internal: return "Internal error of the library";
        }
        return "Unknown code";
    }
}
