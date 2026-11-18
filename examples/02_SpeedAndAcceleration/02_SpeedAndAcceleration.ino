/*==========================================================================
 * 02 SpeedAndAcceleration: maximum speed and acceleration of a motor
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

void setup()
{
    motor
        .setMaxSpeed(20'000)      // steps/s
        .setAcceleration(20'000); // steps/s^2: the maximum speed is reached after 1 s

    motor.setPosition(0); // call the current position 0
}

void loop()
{
    motor.moveAbs(40'000); // blocks until the motor has arrived
    Serial.printf("position: %d\n", motor.getPosition());
    delay(200);

    motor.moveAbs(0);
    Serial.printf("position: %d\n", motor.getPosition());
    delay(200);
}
