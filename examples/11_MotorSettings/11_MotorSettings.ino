/*==========================================================================
 * 11 MotorSettings: adapt the library to the motor and its driver
 *
 * The settings that describe the driver (pulse width, polarities, DIR setup
 * time, enable) are made once in setup(), while the motor stands still.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

void setup()
{
    motor
        .setPulseWidth(10)          // this driver needs pulses of at least 10 us
        .setStepPinPolarity(LOW)    // and steps on the falling edge
        .setInverseRotation(true)   // positive direction = away from the motor
        .setDirSetupTime(5)         // us
        .setEnable(6, LOW, 50, 500) // pin 6, active low; on 50 ms before a move, off 500 ms after it
        .setPullInSpeed(400)        // steps/s
        .setMaxSpeed(30'000)        // steps/s
        .setAcceleration(80'000)    // steps/s^2
        .setDeceleration(40'000);   // steps/s^2: brake more gently than accelerate
}

void loop()
{
    motor.moveRel(20'000);
    delay(300);

    motor.setMaxSpeed(5'000); // a slow way back
    motor.moveRel(-20'000);
    motor.setMaxSpeed(30'000);
    delay(300);
}
