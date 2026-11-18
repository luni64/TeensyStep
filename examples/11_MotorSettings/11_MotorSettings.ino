/*==========================================================================
 * 11 MotorSettings: adapt the library to the motor and its driver
 *
 * Besides maximum speed and acceleration, a motor has these settings:
 *
 * - setPullInSpeed(v):       speed up to which the motor can start and stop
 *                            without a ramp. The motor starts at this speed
 *                            and accelerates from there. Default 100 steps/s.
 * - setPulseWidth(us):       width of the step pulse in microseconds; see the
 *                            data sheet of the driver. Default 5 us.
 * - setStepPinPolarity(p):   HIGH: the driver steps on a positive pulse (default),
 *                            LOW:  on a negative pulse
 * - setInverseRotation(b):   true: swaps the direction of the motor
 * - setDirSetupTime(us):     time the driver needs between a change of the
 *                            DIR signal and the next step pulse
 * - setEnablePin(pin, ...):  optional. If the driver has an enable input, the
 *                            library switches the driver on before a move and
 *                            off after it. Without this call the motor works
 *                            the same way, and the driver stays on.
 * - setDeceleration(a):      optional. A different value for decelerating;
 *                            without it, the acceleration is used for both.
 *
 * Maximum speed, acceleration, deceleration and pull-in speed can be changed
 * at any time, also during a move. The other settings describe the hardware;
 * set them once in setup() while the motor stands still.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

void setup()
{
    motor
        .setPulseWidth(10)        // this driver needs pulses of at least 10 us
        .setStepPinPolarity(LOW)  // and steps on the falling edge
        .setInverseRotation(true) // positive direction = away from the motor
        .setDirSetupTime(5)       // us
        .setEnablePin(6, LOW, 50, 500) // pin 6, active low; on 50 ms before a move, off 500 ms after it
        .setPullInSpeed(400)      // steps/s
        .setMaxSpeed(30'000)      // steps/s
        .setAcceleration(80'000)  // steps/s^2
        .setDeceleration(40'000); // steps/s^2: brake more gently than accelerate
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
