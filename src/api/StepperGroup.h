#pragma once

#include "../backend/Backend.h"
#include "../core/errorCode.h"
#include "Stepper.h"
#include <cstdint>
#include <functional>
#include <initializer_list>

namespace TeensyStep
{
    /// @brief Motors that move synchronized.
    ///
    /// @details The members of a group start together, arrive together and stay on a straight line between
    /// start and target at all times, also while accelerating and when the group is stopped early. The group
    /// is as fast as possible without any member exceeding its own maximum speed or acceleration.
    ///
    /// A group is a light object: a list of motors. It can be defined once or created where it is needed,
    /// also as a temporary (`StepperGroup{x, y}.moveAbs({100, 200});`); a move that was started goes on when
    /// the group object is gone. A motor can be a member of several groups, but it can move in only one at a
    /// time: a command for a group with a busy member returns motorBusy.
    ///
    /// While a group moves, the maximum speed and the acceleration of its members can be changed. New
    /// targets are accepted when the group stands still.
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

        // Moves on a straight line. The targets are given in the order of the members, or taken from the
        // members (Stepper::setTargetAbs / setTargetRel; a member without a target stays where it is).
        errorCode moveAbsAsync(std::initializer_list<int32_t> targets);
        errorCode moveAbsAsync(const int32_t* targets, unsigned n);
        errorCode moveAsync();
        errorCode moveAbs(std::initializer_list<int32_t> targets); ///< returns when the group has arrived
        errorCode moveAbs(const int32_t* targets, unsigned n);
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

        Stepper* motors[maxMembers] = {};
        unsigned count              = 0;
        void (*arrival)()           = nullptr;
        float baseSpeed[maxMembers] = {}; // speeds given to rotateAsync(), for overrideSpeed()
        bool hasBaseSpeed           = false;

        // as the version 2 controllers use a group
        float speedFactor   = 1;     // maximum speeds of the members are scaled by this factor
        bool unlimitedSpeed = false; // the maximum speeds of the members are no limit

        friend class StepControl;
        friend class RotateControl;
    };
}
