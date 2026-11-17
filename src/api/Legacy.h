#pragma once

// The controllers of TeensyStep version 2, kept so that existing sketches compile and run. They are thin
// wrappers around StepperGroup and deprecated; new sketches give the commands to Stepper and StepperGroup.
//
//   version 2                                 now
//   StepControl c; c.move(m);                 m.moveAbs(target);
//   c.moveAsync(m1, m2);                      StepperGroup{m1, m2}.moveAsync();
//   RotateControl r; r.rotateAsync(m);        m.rotateAsync(speed);
//   r.overrideSpeed(0.5);                     m.rotateAsync(newSpeed);
//   c.isRunning()                             m.isMoving()

#include "Stepper.h"
#include "StepperGroup.h"
#include <cstddef>

// the two classes use each other: no deprecation warnings from inside this file
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace TeensyStep
{
    /// @deprecated Version 2: moves motors to the targets set with Stepper::setTargetAbs / setTargetRel.
    class [[deprecated("TeensyStep 2 controller: use the move commands of Stepper and StepperGroup")]] StepControl
    {
     public:
        /// @param pulseWidth      width of the step pulses in microseconds, set on the motors this controller moves
        /// @param accUpdatePeriod not used any more
        StepControl(unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000) : pulseWidth(pulseWidth) { (void)accUpdatePeriod; }

        // non-blocking: motors as a list, e.g. moveAsync(m1, m2), or as an array of pointers; optionally with a speed factor first
        template <typename... Steppers> void moveAsync(Steppers&... steppers) { moveAsync(1.0f, steppers...); }
        template <typename... Steppers> void moveAsync(float speedOverride, Steppers&... steppers)
        {
            if (attach(speedOverride, steppers...)) group.moveAsync();
        }
        template <size_t N> void moveAsync(Stepper* (&steppers)[N]) { moveAsync(1.0f, steppers); }
        template <size_t N> void moveAsync(float speedOverride, Stepper* (&steppers)[N])
        {
            if (attach(speedOverride, steppers)) group.moveAsync();
        }

        // blocking
        template <typename... Steppers> void move(Steppers&... steppers) { move(1.0f, steppers...); }
        template <typename... Steppers> void move(float speedOverride, Steppers&... steppers)
        {
            if (attach(speedOverride, steppers...)) group.move();
        }
        template <size_t N> void move(Stepper* (&steppers)[N]) { move(1.0f, steppers); }
        template <size_t N> void move(float speedOverride, Stepper* (&steppers)[N])
        {
            if (attach(speedOverride, steppers)) group.move();
        }

        void stopAsync() { group.stopAsync(); }
        void stop() { group.stop(); }
        void emergencyStop() { group.emergencyStop(); }

        bool isRunning() const { return group.isMoving(); }
        bool isOk() const { return true; }
        int getCurrentSpeed() const { return leadSpeed(group); }
        void setCallback(void (*callback)()) { group.onArrival(callback); } ///< called when the targets are reached

        StepControl(const StepControl&)            = delete;
        StepControl& operator=(const StepControl&) = delete;

     protected:
        friend class RotateControl;

        static int leadSpeed(const StepperGroup& g) // speed of the fastest motor of the group
        {
            float lead = 0;
            for (unsigned i = 0; i < g.count; i++)
            {
                float v = g.motors[i]->getSpeed();
                if ((v < 0 ? -v : v) > (lead < 0 ? -lead : lead)) lead = v;
            }
            return (int)lead;
        }

        static bool fill(StepperGroup& g, unsigned width)
        {
            for (unsigned i = 0; i < g.count; i++) g.motors[i]->setPulseWidth((float)width);
            return true;
        }

        template <typename... Steppers> bool attach(float speedOverride, Steppers&... steppers)
        {
            if (group.isMoving()) return false; // as in version 2: a controller that is moving takes no new move
            group.clear();
            for (Stepper* s : {&steppers...}) group.add(*s);
            group.speedFactor = speedOverride;
            return fill(group, pulseWidth);
        }

        template <size_t N> bool attach(float speedOverride, Stepper* (&steppers)[N])
        {
            if (group.isMoving()) return false;
            group.clear();
            for (size_t i = 0; i < N; i++) group.add(*steppers[i]);
            group.speedFactor = speedOverride;
            return fill(group, pulseWidth);
        }

        StepperGroup group;
        unsigned pulseWidth;
    };

    /// @deprecated Version 2: rotates motors at the speeds set with Stepper::setMaxSpeed (negative: backwards),
    /// keeping the ratio of the speeds.
    class [[deprecated("TeensyStep 2 controller: use rotateAsync() of Stepper and StepperGroup")]] RotateControl
    {
     public:
        RotateControl(unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000) : pulseWidth(pulseWidth) { (void)accUpdatePeriod; }

        template <typename... Steppers> void rotateAsync(Steppers&... steppers)
        {
            if (group.isMoving()) return;
            group.clear();
            for (Stepper* s : {&steppers...}) group.add(*s);
            start();
        }
        template <size_t N> void rotateAsync(Stepper* (&steppers)[N])
        {
            if (group.isMoving()) return;
            group.clear();
            for (size_t i = 0; i < N; i++) group.add(*steppers[i]);
            start();
        }

        void overrideSpeed(float factor) { group.overrideSpeed(factor); } ///< factor of the speeds set with setMaxSpeed; may be above 1
        void overrideAcceleration(float factor)                            ///< factor of the accelerations the motors had at rotateAsync()
        {
            for (unsigned i = 0; i < group.count; i++) group.motors[i]->setAcceleration(baseAcceleration[i] * factor);
        }

        void stopAsync() { group.stopAsync(); }
        void stop() { group.stop(); }
        void emergencyStop() { group.emergencyStop(); }

        bool isRunning() const { return group.isMoving(); }
        bool isOk() const { return true; }
        int getCurrentSpeed() const { return StepControl::leadSpeed(group); }
        void setCallback(void (*callback)()) { group.onArrival(callback); }

        RotateControl(const RotateControl&)            = delete;
        RotateControl& operator=(const RotateControl&) = delete;

     protected:
        void start()
        {
            float speeds[StepperGroup::maxMembers];
            for (unsigned i = 0; i < group.count; i++)
            {
                Stepper* s = group.motors[i];
                s->setPulseWidth((float)pulseWidth);
                speeds[i]           = s->reversed ? -s->vMax : s->vMax;
                baseAcceleration[i] = s->acc;
            }
            group.unlimitedSpeed = true; // in version 2 the speed factor may exceed 1: the maximum speed is the base, not a limit
            group.rotateAsync(speeds, group.count);
        }

        StepperGroup group;
        unsigned pulseWidth;
        float baseAcceleration[StepperGroup::maxMembers] = {};
    };
}

#pragma GCC diagnostic pop
