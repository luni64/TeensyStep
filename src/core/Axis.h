#pragma once

#include "errorCode.h"
#include <cstdint>

namespace TeensyStep
{
    namespace core
    {
        /// @brief One event of an axis, as produced by Axis::next(): wait for `delay`, then make a step (or none).
        struct StepEvent
        {
            float delay;  ///< seconds from the previous event of this axis to this event (from now, if the axis stood still)
            int8_t dir;   ///< step to make at the event. +1: the position counts up, -1: it counts down, 0: no step, wait only
            bool arrived; ///< this step reaches the target of the current move command
        };

        /// @brief Motion of one motor: takes commands and produces the steps, one at a time. No hardware access.
        ///
        /// @details A command sets a goal: a target position with the speed to have there (moveAbs, moveRel),
        /// or a speed (rotate, stop). next() decides every step anew from the current position, the current
        /// speed and the goal. Commands and limits can therefore change at any time, also during a move.
        ///
        /// The axis reaches a target position exactly, at exactly the given speed, and then keeps that state:
        /// it stands still for speed 0 and runs on at the speed otherwise. If the target cannot be reached
        /// directly, the axis takes a run-up, with at most two reversals (at most one for speed 0).
        ///
        /// Speeds are in steps/s, accelerations in steps/s², positions in steps. The ramp is linear: constant
        /// acceleration and deceleration. Up to the pull-in speed the motor starts, stops and reverses
        /// without a ramp.
        ///
        /// No event waits longer than the maximum delay (setMaxDelay). A longer step period is handed out as
        /// several events, of which only the last one makes the step. At every event the axis notices a new
        /// command or limit and decides again, so that a command takes effect within the maximum delay also
        /// at very low speeds. A backend with a limited timer range sets the maximum delay to that range.
        ///
        /// The functions are not protected against each other: a caller that uses next() in an interrupt
        /// has to keep the interrupt from running during the other calls.
        class Axis
        {
         public:
            static constexpr float maxSpeedLimit        = 1'000'000.0f;     ///< highest speed, steps/s
            static constexpr float minSpeedLimit        = 0.001f;           ///< lowest speed, steps/s
            static constexpr float maxAccelerationLimit = 1'000'000'000.0f; ///< highest acceleration, steps/s²
            static constexpr float minAccelerationLimit = 0.001f;           ///< lowest acceleration, steps/s²
            static constexpr float noDelayLimit         = 3.0e38f;          ///< for setMaxDelay(): never split a step period

            Axis();

            // Limits. Values outside the supported range are limited to it; the function then returns the
            // warning speedLimited or accelerationLimited. Allowed at any time.
            errorCode setMaxSpeed(float stepsPerSec);       ///< default 800
            errorCode setAcceleration(float stepsPerSec2);  ///< default 2500; also sets the deceleration, until setDeceleration() is used
            errorCode setDeceleration(float stepsPerSec2);
            errorCode setPullInSpeed(float stepsPerSec);    ///< default 100

            /// @brief Longest delay of one event, in seconds. Default 0.001, lowest value 0.00001.
            void setMaxDelay(float seconds);

            /// @brief Switches the run-up off (default: on). Without it the axis never reverses to reach a
            /// target at its speed: if the way is too short, it arrives slower (too short to accelerate) or
            /// faster (too short to decelerate) and then goes on to the speed. moveAbs() returns the warning
            /// targetSpeedNotReached in that case, also for a target at which the axis cannot stop in time.
            /// Used for the path of a group, which must not leave its line.
            void setRunUp(bool on) { runUp = on; }

            /// @brief Sets the current speed (sign: direction), without a ramp. For a caller that changes
            /// what a step of this axis is: the path of a group at the start of a new segment.
            void setSpeed(float stepsPerSec);

            // The same for a caller that prepares a move ahead and starts it from the step calculation (the
            // path of a group at the arrival of the move before): what takes time is done by makeLimits()
            // and squared(), in the context of the caller; setLimits(), scaleSpeed() and startMove() only
            // copy and multiply.

            /// @brief The four limits as the axis keeps them.
            struct Limits
            {
                int64_t wMax, twoA, twoD, wPull, maxStepsA, maxStepsD;
                float vMax, vPull;
            };
            static Limits makeLimits(float maxSpeed, float acceleration, float deceleration, float pullInSpeed);
            void setLimits(const Limits& limits);
            static int64_t squared(float stepsPerSec); ///< a speed as the axis keeps it

            /// @brief Multiplies the current speed by `factor` (> 0), without a ramp.
            void scaleSpeed(float factor);

            /// @brief moveAbs() for a target in the direction of counting up, without the checks:
            /// `speedAtTarget` >= 0 and not above the maximum speed, `squaredSpeed` = squared(speedAtTarget).
            void startMove(int32_t pos, float speedAtTarget, int64_t squaredSpeed);

            // Commands. Allowed at any time; a new command replaces the current one.

            /// @brief Move to `pos` and have the speed `speedAtTarget` there. The sign of the speed is the
            /// direction in which the target is passed. 0: stop at the target.
            /// @return speedLimited if the speed is outside the range or above the maximum speed
            errorCode moveAbs(int32_t pos, float speedAtTarget = 0);
            errorCode moveRel(int32_t delta, float speedAtTarget = 0); ///< like moveAbs, relative to the current position
            errorCode rotate(float stepsPerSec);                       ///< run at this speed (sign: direction); 0 stops
            errorCode stop();                                          ///< decelerate to standstill
            void emergencyStop();                                      ///< stand still at once, without deceleration

            /// @brief Holds the axis (true) or lets it go on (false). An axis that is held decelerates to
            /// standstill and stands, whatever its command asks for; the command stays. On the way it still
            /// arrives at a target: one that it shall pass is passed slower than asked for. While it stands,
            /// next() hands out events without a step, none longer than a millisecond, so that it goes on
            /// within that time. An axis without a command is idle, held or not.
            void setHold(bool on)
            {
                if (held == on) return;
                held = on;
                version++;
            }
            bool isHeld() const { return held; }

            /// @brief Sets the position counter. Does not move the axis; a target stays where it is.
            void setPosition(int32_t pos)
            {
                x = pos;
                version++;
            }

            /// @brief Calculates the next event. To be called once per event, when the previous one is due.
            /// @return false if there is nothing to do (the axis stands still)
            bool next(StepEvent& event);

            // State
            int32_t position() const { return x; }               ///< position after the last step from next()
            float speed() const { return sigma * vCur; }         ///< speed at the last step, steps/s; 0 at standstill
            bool isMoving() const { return mode != Mode::idle; } ///< a command is being carried out

         protected:
            enum class Mode : uint8_t {
                idle,
                position, // goal: position xT with speed sT * sqrt(wT)
                velocity, // goal: speed sG * sqrt(wG)
            };

            struct Plan
            {
                enum Kind : uint8_t {
                    idle, // nothing to do
                    halt, // stand still now, without a step, then decide again
                    step,
                    wait, // held at standstill: an event without a step, then decide again
                } kind;
                int8_t dir;
                int64_t w;     // squared speed after the step
                bool arrived;  // the step reaches the commanded target
                bool thenStop; // standstill after the step
            };

            bool decide(float doneBefore, int8_t dirBefore);
            float speedOf(int64_t wNew) const;
            Plan planPosition();
            Plan planVelocity();
            Plan planHold();
            Plan approachStop(int32_t p, bool isMain);
            Plan stepToMovingTarget(int64_t rho, int64_t wTe);
            Plan stepWithoutRunUp(int64_t rho, int64_t wTe);
            bool canStopIn(int64_t rho) const;
            Plan brake();
            bool feasible(int64_t w0, int64_t rho, int64_t wTe) const;
            int64_t runUpSteps(int64_t wTe) const;
            int64_t faster(int64_t limit) const;
            void standStill();

            // All speeds are kept as squared speeds w = v², in steps²/s² with 20 fractional bits. Constant
            // acceleration changes w by the same amount on every step (2a), so the ramp needs only additions.
            int32_t x     = 0; // position
            int64_t w     = 0; // squared speed at the last step
            int8_t sigma  = 0; // direction of motion, 0: standstill
            float vCur    = 0; // sqrt(w), steps/s

            Mode mode  = Mode::idle;
            int32_t xT = 0; // position mode: target
            int64_t wT = 0; //                squared speed at the target
            int8_t sT  = 0; //                direction at the target, 0: stop there
            int64_t wG = 0; // velocity mode: squared speed
            int8_t sG  = 0; //                direction, 0: stop

            int64_t wMax, twoA, twoD, wPull; // limits: v², 2a, 2d, pull-in v²
            float vMax;                      // maximum speed as given
            int64_t maxStepsA, maxStepsD;    // step counts above which n * 2a and n * 2d exceed every squared speed

            float vPull   = 0; // pull-in speed
            float vTarget = 0; // position mode: speed at the target, as given
            float vGoal   = 0; // velocity mode: speed, as given

            int64_t wDelay = -1; // step period at constant speed: valid for this squared speed
            float delay    = 0;

            // The step that is waiting. It is counted when its last event is handed out; until then a new
            // command can still change or cancel it.
            bool pending         = false;
            Plan pend            = {Plan::idle, 0, 0, false, false};
            float pendV          = 0; // speed after the step
            float pendDone       = 0; // part of the step period that had passed when the events below were planned
            float pendPiece      = 0; // delay of each remaining event
            uint32_t pendPieces  = 1; // events planned for the rest of the period, the last one makes the step
            uint32_t pendLeft    = 0; // events not handed out yet
            uint32_t pendVersion = 0; // version when the step was decided
            uint32_t version     = 0; // counts commands and changes of the limits
            float maxDelay       = 0.001f;

            // n * 2a and n * 2d (n >= 0), limited to a value above every squared speed; without a division
            int64_t timesTwoA(int64_t n) const { return n > maxStepsA ? ((int64_t)1 << 61) : n * twoA; }
            int64_t timesTwoD(int64_t n) const { return n > maxStepsD ? ((int64_t)1 << 61) : n * twoD; }
            bool separateDeceleration = false;
            bool runUp                = true;
            bool held                 = false; // setHold()

            // Steps at constant speed. When a step is decided that does not change the speed, the number of
            // steps after it that would be decided in exactly the same way is known. next() hands them out
            // without deciding them, as long as no command and no limit changes (`version`).
            int32_t constantSteps() const;
            int64_t headingRho   = -1; // of the step decided last: steps to the point the axis is heading for, this one included; -1: not known
            int64_t headingFloor = 0;  // squared speed from which the axis may get to that point without a further ramp
            int32_t coastLeft     = 0;    // steps that can still be handed out this way
            uint32_t coastVersion = 0;    // valid for this version
            bool coastEnabled     = true; // false: every step is decided (for the tests)
        };
    }
}
