#pragma once

#include "Axis.h"
#include "errorCode.h"
#include <cstdint>

namespace TeensyStep
{
    namespace core
    {
        /// @brief Limits of one motor, in steps/s and steps/s².
        struct MotorLimits
        {
            float maxSpeed     = 800;
            float acceleration = 2500;
            float deceleration = 2500;
            float pullInSpeed  = 100;
        };

        /// @brief One event of a group, as produced by Group::next(): wait for `delay`, then step the members
        /// named in `stepMask`.
        struct GroupEvent
        {
            float delay;       ///< seconds from the previous event of this group to this event (from now, if the group stood still)
            uint16_t stepMask; ///< bit i: member i makes a step at the event. 0: no step, wait only
            uint16_t dirMask;  ///< bit i: member i counts up when it steps (1) or down (0)
            bool arrived;      ///< this event reaches the targets of the current move
        };

        /// @brief Motion of several motors that move synchronized. No hardware access.
        ///
        /// @details The members move along a straight line between their start positions and their targets
        /// (moveAbs), or with a fixed ratio of their speeds (rotate). One ramp is calculated for the path as
        /// a whole; it is the ramp of the member with the longest way, which makes a step at every step of
        /// the path. The other members step whenever they have fallen half a step behind. No member is
        /// therefore ever more than half a step off the line, whatever the path does: accelerate, stop
        /// early, or run past the targets and return.
        ///
        /// The path is as fast as the members allow: none of them exceeds its own maximum speed,
        /// acceleration, deceleration or pull-in speed. The group can have a maximum speed, an acceleration
        /// and a deceleration of its own, for the path: the speed and acceleration of the member with the
        /// longest way. The limits of the members stay ceilings.
        ///
        /// A move can have a speed at the target, again that of the member with the longest way. The group
        /// then passes its targets at this speed and runs on along the line. Unlike a single motor it
        /// never takes a run-up: if the way is too short, it arrives as close to the speed as it can.
        ///
        /// The limits of a member and of the group can change at any time. New targets are accepted while
        /// the group stands still and when it has passed its targets and runs on; the new move then starts
        /// where the members are, without a gap in time (a segment of a longer path). The speeds of the
        /// single members jump there; the caller has to keep the speed at such a point low enough. Within
        /// a move, only the speed at the target can change. A new speed ratio is accepted while the group
        /// stands still; a rotating group can change its speed as a whole.
        class Group
        {
         public:
            static constexpr unsigned maxMembers = 16;

            Group() { path.setRunUp(false); }

            /// @brief Sets the number of members (1 to maxMembers) and resets them to position 0 and the
            /// default limits. @return invalidArgument for another number, notSupported while moving
            errorCode setMembers(unsigned count);
            unsigned members() const { return count; }

            void setLimits(unsigned member, const MotorLimits& limits); ///< allowed at any time
            void setPosition(unsigned member, int32_t pos);             ///< sets the position counter of a member
            void setMaxDelay(float seconds) { path.setMaxDelay(seconds); } ///< see Axis::setMaxDelay()

            // Speed, acceleration and deceleration of the path. 0: none, the path is as fast as the members
            // allow. Allowed at any time. During a move they return the warning speedLimited or
            // accelerationLimited if the members allow less; the smaller value is used.
            errorCode setMaxSpeed(float stepsPerSec);
            errorCode setAcceleration(float stepsPerSec2); ///< also sets the deceleration, until setDeceleration() is used
            errorCode setDeceleration(float stepsPerSec2);

            /// for speedAtStart of moveAbs(): the path keeps the speed it has
            static constexpr float keepSpeed = __builtin_nanf("");

            /// @brief Move all members to their targets (one per member) on a straight line.
            /// @param speedAtTarget speed of the path at the targets (the member with the longest way).
            ///        0: stop there. Otherwise the group passes the targets at this speed and runs on.
            /// @param speedAtStart speed of the path at the start of the move. Default: the speed it has
            ///        (0 for a group that stands still).
            /// @details While the group is on its way to targets, the same targets with another speed at
            /// the target are accepted, other targets are not.
            /// @return notSupported for other targets while the group is on its way, or while it rotates.
            ///         invalidArgument for a negative speed. speedLimited / accelerationLimited if the
            ///         members allow less than the group asks for. targetSpeedNotReached if the way is too
            ///         short for the speed at the target
            errorCode moveAbs(const int32_t* targets, float speedAtTarget = 0, float speedAtStart = keepSpeed);

            /// @brief Rotate the members at these speeds (one per member, sign: direction). The ratio of
            /// the speeds is kept exactly. While the group rotates, speeds with the same ratio change the
            /// speed of the group; another ratio returns notSupported. All speeds 0: stop.
            /// @return speedLimited if a member would exceed its maximum speed; the group is then slower, with the same ratio
            errorCode rotate(const float* speeds);

            /// @brief Runs a rotating group at `factor` times the speeds given to rotate(). Negative: all
            /// members reverse. 0: stop; a later factor starts the group again.
            errorCode overrideSpeed(float factor);

            errorCode stop();     ///< decelerate to standstill, all members stay on the line
            void emergencyStop(); ///< stand still at once

            /// @brief Calculates the next event. @return false if there is nothing to do
            bool next(GroupEvent& event);

            int32_t position(unsigned member) const { return m[member].pos; }
            float speed(unsigned member) const; ///< current speed of a member, steps/s
            bool isMoving() const { return path.isMoving(); }

            /// @brief The motion along the path: its position counts the steps of the member with the longest way.
            const Axis& lead() const { return path; }

         protected:
            enum class Kind : uint8_t {
                none,
                move,
                rotate,
            };

            struct Member
            {
                int32_t pos    = 0;
                int32_t target = 0; // of the current move
                uint32_t share = 0; // steps of this member per `length` steps of the path
                uint32_t err   = 0; // share * path position + length / 2, modulo length
                int8_t dir     = 1; // direction of this member when the path moves forward
                MotorLimits limits;
            };

            void applyLimits();
            errorCode limitWarning() const;
            void startPath(uint32_t pathLength);

            Axis path;                // the ramp of the group; one step of the path is one step of the member with the longest way
            Member m[maxMembers];
            unsigned count     = 1;
            uint32_t length    = 0;   // length of the path in steps (rotation: a fixed large number)
            Kind kind          = Kind::none;
            float baseSpeed    = 0;   // rotation: speed of the fastest member as given
            uint16_t forward   = 0;   // direction bits of the members when the path moves forward
            uint16_t all       = 1;   // bits of all members
            uint16_t lastDirs  = 0;   // direction bits of the last event
            bool passed        = false; // move: the targets are passed, the group runs on

            float ownSpeed = 0, ownAcc = 0, ownDec = 0; // limits of the path as given for the group, 0: none
            bool separateDeceleration = false;
            bool speedCapped = false, accCapped = false, decCapped = false; // the members allow less than the group asks for
        };
    }
}
