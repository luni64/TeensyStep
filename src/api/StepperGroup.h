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
    /// is as fast as possible without any member exceeding its own maximum speed or acceleration; it can be
    /// given a maximum speed and an acceleration of its own (setMaxSpeed, setAcceleration).
    ///
    /// A group is a light object: a list of motors. It can be defined once or created where it is needed,
    /// also as a temporary (`StepperGroup{x, y}.moveAbs({100, 200});`); a move that was started goes on when
    /// the group object is gone. A motor can be a member of several groups, but it can move in only one at a
    /// time: a command for a group with a busy member returns motorBusy.
    ///
    /// While a group moves, the maximum speed and the acceleration of its members and of the group can be
    /// changed. New targets are accepted when the group stands still.
    ///
    /// A group always stops at its targets. Moves that follow each other without a stop (a path through
    /// several points) are made with advanced::GroupPath, see TeensyStepAdvanced.h.
    class StepperGroup
    {
     public:
        static constexpr unsigned maxMembers = backend::maxMotors;

        StepperGroup() = default;
        StepperGroup(std::initializer_list<std::reference_wrapper<Stepper>> motors);

        errorCode add(Stepper& motor); ///< invalidArgument if the group is full or has this motor already
        void remove(Stepper& motor);
        void clear();
        unsigned size() const { return count; }

        // Speed and acceleration of the group: those of the member with the longest way in a move, in
        // steps/s and steps/s². The settings of the members stay ceilings: if a member allows less, the
        // group is slower, and the warning speedLimited or accelerationLimited is reported. 0 (default):
        // no value of the group, it is as fast as its members allow. Chainable, allowed at any time. The
        // values belong to this group object and are used for the moves it starts.
        StepperGroup& setMaxSpeed(float stepsPerSec);
        StepperGroup& setAcceleration(float stepsPerSec2); ///< also sets the deceleration, until setDeceleration() is used
        StepperGroup& setDeceleration(float stepsPerSec2);
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

        /// @brief Runs the group at `factor` times the speeds given to rotateAsync(). Negative: all members
        /// reverse. 0: stop; a later factor starts the group again.
        errorCode overrideSpeed(float factor);

        errorCode stopAsync(); ///< decelerate to standstill; the members stay on the line
        errorCode stop();      ///< returns when the group stands still
        void emergencyStop();  ///< stand still at once

        bool isMoving() const; ///< the members are moving as this group

        /// @brief Sets a function that is called when the group has reached its targets. It is called from
        /// an interrupt: keep it short. nullptr removes it.
        void onArrival(void (*callback)()) { arrival = callback; }

     private:
        int runningChannel() const;
        errorCode takeChannel(int& ch);
        errorCode rotate(const float* speeds, unsigned n, bool remember);
        enum class Setting { none, speed, acceleration, deceleration };
        void applySettings(detail::Channel& c, Setting changed);

        // for advanced::GroupPath: moves with a speed at the targets and at the start
        errorCode start(const int32_t* targets, unsigned n, float speedAtTarget, float speedAtStart);
        errorCode startRel(const int32_t* distances, unsigned n, float speedAtTarget, float speedAtStart);
        errorCode changeSpeedAtTarget(float speedAtTarget);

        Stepper* motors[maxMembers] = {};
        unsigned count              = 0;
        void (*arrival)()           = nullptr;
        float ownSpeed = 0, ownAcc = 0, ownDec = 0; // of the group; 0: none
        bool separateDeceleration   = false;
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
