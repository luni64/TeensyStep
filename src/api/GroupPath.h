#pragma once

#include "../core/Group.h"
#include "../core/errorCode.h"
#include "StepperGroup.h"
#include <cstdint>
#include <initializer_list>

namespace TeensyStep
{
    namespace detail
    {
        struct Engine;
    }

    namespace advanced
    {
        /// @brief One move of a GroupPath: a straight line to the targets, with its speeds.
        ///
        /// The speeds are in the unit of the caller. `length` says how long the move is in that unit, for
        /// example in mm; all speeds of the move are then in mm/s and its acceleration in mm/s². The library
        /// converts them for the motors: it knows the ways of the motors in steps from the targets, so it
        /// needs no more than the length. With a length of 0 the speeds are in steps of the motor with the
        /// longest way in the move, like the speeds of a StepperGroup.
        ///
        ///     float length;        // length of the move in the unit of the caller. 0: no unit of its own
        ///     float speed;         // highest speed on the way. 0: as fast as the motors allow
        ///     float speedAtTarget; // 0: stop at the targets
        ///     float acceleration;  // also for slowing down. 0: as much as the motors allow
        using PathMove = core::PathMove;

        /// @brief Moves of a group that follow each other without a stop: a path through several points.
        ///
        /// @details For programs that calculate a path, for example a motion planner. A sketch that moves
        /// a group from one position to the next needs StepperGroup only.
        ///
        /// The moves are given ahead: one runs, one waits. When the group arrives at the targets of a move
        /// with a speed, it goes on with the move that waits, without a gap in time and without a call of
        /// the sketch in between. The speed along the path stays the same there, in the unit of the
        /// caller. Within each move the members stay on a straight line, as in every move of a group.
        ///
        /// What the caller has to take care of:
        /// - Where two moves meet, the speeds of the single motors jump, without a ramp. The speed at such
        ///   a point has to be low enough for the motors. The library does not plan ahead and does not
        ///   round corners.
        /// - The next move has to be given while the one before it runs: from loop() while room() is not 0,
        ///   or from the function set with onRoom(). If none waits when the group arrives with a speed, it
        ///   runs on along its line until a move is given, which then starts where the motors are.
        /// - A group never reverses to take a run-up. If the way is too short to get from the speed at its
        ///   start to the speed at its targets, the group arrives as close to it as it can, and add()
        ///   returns the warning targetSpeedNotReached.
        /// - The length of a move is taken as given. A wrong length makes all speeds of the move wrong by
        ///   the same factor; the limits of the motors stay ceilings.
        ///
        /// Speed and acceleration of a move are given with the move; those of the StepperGroup
        /// (setMaxSpeed, setAcceleration) are not used for the moves of a path. Arrival function, stop and
        /// state are those of the StepperGroup. The group has to exist as long as the GroupPath.
        class GroupPath
        {
         public:
            explicit GroupPath(StepperGroup& group) : g(group) {}
            ~GroupPath();
            GroupPath(const GroupPath&)            = delete;
            GroupPath& operator=(const GroupPath&) = delete;

            /// @brief Adds a move to the positions `targets`, in the order of the members of the group. It
            /// starts at once if the group stands still or runs on behind its last targets, and waits if
            /// a move is on its way. Returns at once.
            /// @return notSupported if a move is waiting already (see room()), and while the group is
            ///         stopping or rotates. invalidArgument for a wrong number of targets, a negative
            ///         value, or a waiting move without a way. speedLimited / accelerationLimited if the
            ///         motors allow less than the move asks for. targetSpeedNotReached if the way is too
            ///         short for its speeds. motorBusy, noTimer as for a move of the group
            errorCode add(std::initializer_list<int32_t> targets, const PathMove& move);
            errorCode add(const int32_t* targets, unsigned n, const PathMove& move);

            /// @brief Moves that add() takes now: 2 (the group stands still or runs on behind its targets),
            /// 1 (a move is on its way, none waits) or 0.
            unsigned room() const;

            /// @brief Sets a function that is called when a move of the path has arrived, which is when
            /// there is room for a move again. It gets this object and `context`, a pointer of the caller.
            /// It is called from an interrupt: keep it short. nullptr removes it.
            void onRoom(void (*function)(GroupPath& path, void* context), void* context = nullptr);

            /// @brief Runs the path at `factor` times the speeds of its moves (0 <= factor <= 1): the move
            /// under way, the one that waits and all that follow, until another factor is set.
            /// @details 0 holds the path: the group slows down with the acceleration of its moves, goes on
            /// into the moves that follow while it does, and stands on the path. Its moves are kept and
            /// further ones are taken; the group counts as moving. A factor above 0 lets it go on, stop()
            /// of the group gives the path up.
            /// @return invalidArgument for another factor
            errorCode overrideSpeed(float factor);

            /// @brief Current speed along the path, in the unit of the move under way. 0 at standstill.
            float getSpeed() const;

            /// @brief Number of moves of this path that have arrived at their targets, since the object exists.
            uint32_t movesDone() const { return done; }

         private:
            void moveArrived(); // called by the library at the arrival of a move of this path

            StepperGroup& g;
            void (*roomFunction)(GroupPath&, void*) = nullptr;
            void* roomContext                       = nullptr;
            volatile uint32_t done                  = 0;
            float factor                            = 1;

            friend struct detail::Engine;
        };
    }
}
