#pragma once

#include "../backend/Backend.h"
#include "../core/errorCode.h"
#include "Stepper.h"
#include <cstdint>
#include <functional>
#include <initializer_list>

namespace TeensyStep
{
    namespace detail
    {
        struct Channel;
    }
    namespace advanced
    {
        class GroupPath;
    }

    /// @brief Motors that move synchronized.
    ///
    /// @details The members of a group start together, arrive together and stay on a straight line between
    /// start and target at all times, also while accelerating and when the group is stopped early. The group
    /// is as fast as possible without any member exceeding its own maximum speed or acceleration;
    /// overrideSpeed() runs it at a fraction of that speed.
    ///
    /// A group is a light object: a list of motors. It can be defined once or created where it is needed,
    /// also as a temporary (`StepperGroup{x, y}.moveAbs({100, 200});`); a move that was started goes on when
    /// the group object is gone. A motor can be a member of several groups, but it can move in only one at a
    /// time: a command for a group with a busy member returns motorBusy.
    ///
    /// While a group moves, the maximum speed and the acceleration of its members and the speed factor of
    /// the group can be changed. New targets are accepted when the group stands still.
    ///
    /// A group always stops at its targets. Moves that follow each other without a stop (a path through
    /// several points) are given with advanced::GroupPath, see TeensyStepAdvanced.h.
    class StepperGroup
    {
     public:
        static constexpr unsigned maxMembers = backend::maxMotors;

        StepperGroup() = default;
        StepperGroup(const StepperGroup&) = default; ///< a copy is a group of its own with the same members and settings
        StepperGroup& operator=(const StepperGroup&) = default;
        ~StepperGroup(); ///< a move that was started goes on
        StepperGroup(std::initializer_list<std::reference_wrapper<Stepper>> motors);

        errorCode add(Stepper& motor); ///< invalidArgument if the group is full or has this motor already
        void remove(Stepper& motor);
        void clear();
        unsigned size() const { return count; }

        float getSpeed() const; ///< current speed of the group: that of its fastest member, steps/s; 0 at standstill

        // Moves on a straight line. The targets are given in the order of the members: as positions
        // (moveAbs) or as distances from where the members stand (moveRel). Or they are taken from the
        // members (move; Stepper::setTargetAbs / setTargetRel; a member without a target stays where it is).
        //
        // The members remember their targets, and the last one given wins: moveAbs and moveRel replace
        // targets set on the members before, and a target set on a member afterwards replaces the one of
        // the group for the next move(). A command that is refused changes no target.
        errorCode moveAbsAsync(std::initializer_list<int32_t> targets);
        errorCode moveAbsAsync(const int32_t* targets, unsigned n);
        errorCode moveRelAsync(std::initializer_list<int32_t> distances);
        errorCode moveRelAsync(const int32_t* distances, unsigned n);
        errorCode moveAsync();
        errorCode moveAbs(std::initializer_list<int32_t> targets); ///< returns when the group has arrived
        errorCode moveAbs(const int32_t* targets, unsigned n);
        errorCode moveRel(std::initializer_list<int32_t> distances);
        errorCode moveRel(const int32_t* distances, unsigned n);
        errorCode move();

        /// @brief Rotate the members at these speeds (steps/s, sign: direction), in the order of the
        /// members. The ratio of the speeds is kept exactly. While the group rotates, speeds with the same
        /// ratio change the speed of the group; another ratio returns notSupported.
        errorCode rotateAsync(std::initializer_list<float> speeds);
        errorCode rotateAsync(const float* speeds, unsigned n);

        /// @brief The speed factor of the group.
        ///
        /// For moves to targets: the group runs at `factor` times the speed its members allow, 0 to 1
        /// (default 1). The factor can be set at any time, also during a move; it belongs to this group
        /// object and stays for its later moves. 0 holds the group: it decelerates and stands on its line,
        /// the move is not over (isMoving() stays true, a blocking move does not return), and a factor
        /// above 0 lets it go on. stop() ends a move that is held. Another factor returns invalidArgument.
        ///
        /// For a group that was rotated last: the group runs at `factor` times the speeds given to
        /// rotateAsync(). Negative: all members reverse. 0: stop; a later factor starts the group again.
        errorCode overrideSpeed(float factor);

        errorCode stopAsync(); ///< decelerate to standstill; the members stay on the line
        errorCode stop();      ///< returns when the group stands still
        void emergencyStop();  ///< stand still at once

        bool isMoving() const; ///< the members are moving as this group

        /// @brief Sets a function that is called when the group has reached its targets. It is called from
        /// an interrupt: keep it short. The function can have the group as its parameter, so that one
        /// function can serve several groups:
        ///
        ///     void arrived(StepperGroup& group) { ... }
        ///     xy.onArrival(arrived);
        ///     zw.onArrival(arrived);
        ///
        /// It gets the group object that gave the move. If that object is gone when the move arrives (a
        /// group created only for the move), a function with a parameter is not called.
        void onArrival(void (*callback)());
        void onArrival(void (*callback)(StepperGroup& group));
        void onArrival(std::nullptr_t); ///< removes the function

     private:
        int runningChannel() const;
        errorCode takeChannel(int& ch);
        errorCode rotate(const float* speeds, unsigned n, bool remember);

        errorCode start(const int32_t* targets, unsigned n, float speedAtTarget, float speedAtStart);
        errorCode startRel(const int32_t* distances, unsigned n, float speedAtTarget, float speedAtStart);

        Stepper* motors[maxMembers] = {};
        unsigned count              = 0;
        void (*arrival)()           = nullptr;
        void (*arrivalGroup)(StepperGroup&) = nullptr;
        void setArrival(detail::Channel& c); // the functions and this object, for the move that starts
        void callArrival();
        float moveFactor            = 1;     // overrideSpeed() for moves, the last factor above 0
        bool held                   = false; // overrideSpeed(0) for moves
        float baseSpeed[maxMembers] = {}; // speeds given to rotateAsync(), for overrideSpeed()
        bool hasBaseSpeed           = false;

        // as the version 2 controllers use a group
        float speedFactor   = 1;     // maximum speeds of the members are scaled by this factor
        bool unlimitedSpeed = false; // the maximum speeds of the members are no limit

        friend class advanced::GroupPath;
        friend class StepControl;
        friend class RotateControl;
    };
}
