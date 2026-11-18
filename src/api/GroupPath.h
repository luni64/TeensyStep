#pragma once

#include "../core/Group.h"
#include "../core/errorCode.h"
#include "StepperGroup.h"
#include <cstdint>
#include <initializer_list>

namespace TeensyStep
{
    namespace advanced
    {
        /// @brief Moves of a group that follow each other without a stop: a path through several points.
        ///
        /// @details For programs that calculate a path, for example a motion planner. A sketch that moves
        /// a group from one position to the next needs StepperGroup only.
        ///
        /// A move of a GroupPath has a speed at its targets. The group passes the targets at this speed and
        /// runs on along the line until the next command. The next move is given when the targets are
        /// passed, in most cases from the arrival function of the group (StepperGroup::onArrival); it
        /// starts where the members are, without a gap in time. Within each move the members stay on a
        /// straight line, as in every move of a group.
        ///
        /// All speeds are those of the member with the longest way in the move, like the maximum speed of
        /// the group. This member changes from move to move, so the same speed along the path is another
        /// number before and after a corner; speedAtStart gives the second one.
        ///
        /// What the caller has to take care of:
        /// - Where two moves meet, the speeds of the single members jump, without a ramp. The speed at
        ///   such a point has to be low enough for the motors. The library does not plan ahead and does
        ///   not round corners.
        /// - A group never reverses to take a run-up. If the way is too short to reach the speed at the
        ///   targets or to slow down to it, the group arrives as close to it as it can, and the command
        ///   returns the warning targetSpeedNotReached. A move that shall stop at its targets, but is too
        ///   short for it, passes them, stops and returns along the line.
        /// - Other targets are refused (notSupported) while the group is on its way; they are accepted
        ///   when it stands still or has passed its targets.
        ///
        /// Speed, acceleration, arrival function, stop and state are those of the StepperGroup. The group
        /// has to exist as long as the GroupPath.
        class GroupPath
        {
         public:
            /// for speedAtStart: the group keeps the speed it has
            static constexpr float keepSpeed = core::Group::keepSpeed;

            explicit GroupPath(StepperGroup& group) : g(group) {}

            /// @brief Starts a move to the positions `targets` (moveAbsAsync) or over the distances
            /// (moveRelAsync), in the order of the members of the group.
            /// @param speedAtTarget speed at the targets, steps/s. 0: stop there
            /// @param speedAtStart speed at the start of the move, steps/s. Default: the speed the group
            ///        has, so that the member with the longest way starts at the speed the one of the
            ///        move before had
            errorCode moveAbsAsync(std::initializer_list<int32_t> targets, float speedAtTarget = 0, float speedAtStart = keepSpeed);
            errorCode moveAbsAsync(const int32_t* targets, unsigned n, float speedAtTarget = 0, float speedAtStart = keepSpeed);
            errorCode moveRelAsync(std::initializer_list<int32_t> distances, float speedAtTarget = 0, float speedAtStart = keepSpeed);
            errorCode moveRelAsync(const int32_t* distances, unsigned n, float speedAtTarget = 0, float speedAtStart = keepSpeed);

            /// @brief Changes the speed at the targets of the move that is under way: raise it when
            /// further moves follow, set it to 0 to stop at the targets after all.
            /// @return notSupported if the group is not on its way to targets
            errorCode setSpeedAtTarget(float speedAtTarget);

         private:
            StepperGroup& g;
        };
    }
}
