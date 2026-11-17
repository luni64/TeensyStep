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
        /// acceleration, deceleration or pull-in speed.
        ///
        /// The limits of a member can change at any time. New targets and a new speed ratio are accepted
        /// only while the group stands still; a rotating group can change its speed as a whole.
        class Group
        {
         public:
            static constexpr unsigned maxMembers = 16;

            /// @brief Sets the number of members (1 to maxMembers) and resets them to position 0 and the
            /// default limits. @return invalidArgument for another number, notSupported while moving
            errorCode setMembers(unsigned count);
            unsigned members() const { return count; }

            void setLimits(unsigned member, const MotorLimits& limits); ///< allowed at any time
            void setPosition(unsigned member, int32_t pos);             ///< sets the position counter of a member
            void setMaxDelay(float seconds) { path.setMaxDelay(seconds); } ///< see Axis::setMaxDelay()

            /// @brief Move all members to their targets (one per member) on a straight line.
            /// @return notSupported while the group is moving
            errorCode moveAbs(const int32_t* targets);

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
                uint32_t share = 0; // steps of this member per `length` steps of the path
                uint32_t err   = 0; // share * path position + length / 2, modulo length
                int8_t dir     = 1; // direction of this member when the path moves forward
                MotorLimits limits;
            };

            void applyLimits();
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
        };
    }
}
