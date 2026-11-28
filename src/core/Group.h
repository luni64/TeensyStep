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

        /// @brief A move of a path, for Group::addMove(). The speeds are in the unit of the caller: whatever
        /// unit `length` is in, per second. With a length of 0 they are in steps of the member with the
        /// longest way, like all other speeds of a group.
        struct PathMove
        {
            float length        = 0; ///< length of the move in the unit of the caller, for example mm. 0: no unit of its own
            float speed         = 0; ///< highest speed on the way. 0: as fast as the members allow
            float speedAtTarget = 0; ///< 0: stop at the targets
            float acceleration  = 0; ///< also for slowing down. 0: as much as the members allow
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

            /// @brief Changes the speed at the targets of the move that is under way.
            /// @return notSupported if the group is not on its way to targets; otherwise as moveAbs()
            errorCode setSpeedAtTarget(float speedAtTarget);

            // A path that is given ahead: one move runs, one waits. The move that waits is prepared when it
            // is given and started by next() at the arrival of the one before it, so that the path goes on
            // without a call from outside. The speed along the path is kept there in the unit of the
            // caller, whichever members have the longest way in the two moves.

            /// @brief Adds a move to these targets (one per member). It starts at once if the group stands
            /// still or runs on behind its last targets, and waits if a move is on its way.
            /// @return notSupported if a move is waiting already, while the group rotates and while it
            ///         is stopping. invalidArgument for a negative value, and for a waiting move without a
            ///         way. speedLimited / accelerationLimited if the members allow less than the move
            ///         asks for. targetSpeedNotReached if the way is too short to get from the speed at
            ///         its start to the speed at its targets
            errorCode addMove(const int32_t* targets, const PathMove& move);
            unsigned room() const; ///< moves that addMove() takes now: 2, 1 or 0

            // addMove() in two parts, for a caller that keeps next() out with a lock: preparing a move takes
            // time (divisions, double precision), and next() may run during it. prepareWaiting() fills
            // `line` for a move that shall wait and needs no lock; commitWaiting() hands it over and is
            // short. Both refuse if no move is on its way or one waits already: then addMove() is the way.
            struct Line;
            bool prepareWaiting(Line& line, const int32_t* targets, const PathMove& move, errorCode& code) const;
            bool commitWaiting(const Line& line);

            /// @brief Runs the moves of addMove() at `factor` times their speeds (0 <= factor <= 1): the one
            /// under way, the one that waits and those that follow.
            /// @details 0 holds the path: the group decelerates with the acceleration of its moves, goes on
            /// into the move that waits while it does, and stands on the path. It keeps its moves and takes
            /// further ones (isMoving() stays true); a factor above 0 lets it go on, stop() gives the path
            /// up. The factor stays for the paths that follow; moveAbs() and rotate() are not held.
            /// @return invalidArgument for another factor
            errorCode setOverride(float factor);

            float overrideFactor() const { return holding ? 0.0f : feed; }

            /// @brief The same for the moves of moveAbs(): they run at `factor` times the speed that the
            /// members and the group allow (0 <= factor <= 1), and 0 holds the group on its line until
            /// another factor is set. The factor stays for the moves that follow. The moves of addMove() and
            /// a rotation are not affected.
            /// @return invalidArgument for another factor
            errorCode setMoveOverride(float factor);

            /// @brief Current speed along the path, in the unit of the move under way.
            float pathSpeed() const;

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

            // the limits of the path of a line: what the members allow, and what is asked for
            struct PathLimits
            {
                float vMax, acc, dec, pull;
                bool speedCapped, accCapped, decCapped;
            };
            PathLimits limitsOf(const uint32_t* share, uint32_t len, float wantSpeed, float wantAcc, float wantDec, float factor) const;

         public:
            // a move of addMove(), prepared
            struct Line
            {
                uint32_t serial = 0;          // the move it follows
                uint32_t share[maxMembers];
                int32_t target[maxMembers];
                int8_t dir[maxMembers];
                uint32_t length = 0;          // steps of the member with the longest way
                float unit      = 1;          // steps of that member per unit of the caller
                float wantSpeed = 0, wantAcc = 0; // as given, in steps of that member; 0: none
                float exit      = 0;          // speed at the targets as given, in steps of that member
                float vT        = 0;          // the same with the override, not above the maximum speed
                int64_t wT      = 0;
                Axis::Limits limits;
                bool speedCapped = false, accCapped = false;
            };

         protected:
            errorCode prepare(Line& l, const int32_t* from, const int32_t* targets, const PathMove& move, float entrySpeed) const;
            void finish(Line& l) const;
            void activate(const Line& l);

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

            // moves of addMove()
            Line queued;               // the move that waits
            bool hasQueued   = false;
            uint32_t serial  = 0;      // counts the moves that were started
            bool pathMove    = false;  // the move under way came from addMove()
            bool stopping    = false;  // stop() was called
            float unit       = 1;      // of the move under way: steps of the path per unit of the caller
            float moveSpeed = 0, moveAcc = 0; // of the move under way, as given, in steps of the path; 0: none
            float moveExit  = 0;       // its speed at the targets as given, in steps of the path
            float feed      = 1;       // setOverride(), the last factor above 0
            bool holding    = false;   // setOverride(0)
            float ownFeed   = 1;       // setMoveOverride(), the last factor above 0
            bool ownHold    = false;   // setMoveOverride(0)

            float ownSpeed = 0, ownAcc = 0, ownDec = 0; // limits of the path as given for the group, 0: none
            bool separateDeceleration = false;
            bool speedCapped = false, accCapped = false, decCapped = false; // the members allow less than the group asks for
        };
    }
}
