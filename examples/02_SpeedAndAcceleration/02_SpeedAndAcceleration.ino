/*==========================================================================
 * 02 SpeedAndAcceleration: motor settings, absolute and relative targets
 *
 * - setMaxSpeed():     highest speed the motor will reach, in steps/s
 * - setAcceleration(): how fast the speed changes, in steps/s^2
 * - moveAbs(p):        move to position p (steps, counted from power-up or setPosition)
 * - moveRel(d):        move d steps from the current position
 *
 * The settings can be chained and can be changed at any time, also while
 * the motor moves (see example 05).
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

void setup()
{
    Serial.begin(115200);

    motor
        .setMaxSpeed(20'000)      // steps/s
        .setAcceleration(50'000); // steps/s^2

    motor.setPosition(0); // call the current position 0
}

void loop()
{
    motor.moveAbs(8000); // blocks until the motor has arrived
    Serial.printf("position: %d\n", motor.getPosition());
    delay(200);

    motor.moveAbs(0);
    Serial.printf("position: %d\n", motor.getPosition());
    delay(200);
}
