/*==========================================================================
 * 05 ChangeTarget: give a new target while the motor is moving
 *
 * Type a position into the serial monitor; the motor follows the latest
 * value. "v 5000" sets a new maximum speed.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

void setup()
{
    motor.setMaxSpeed(15'000).setAcceleration(30'000);
}

void loop()
{
    if (Serial.available())
    {
        if (Serial.peek() == 'v') // "v 5000": new maximum speed, takes effect immediately
        {
            Serial.read();
            motor.setMaxSpeed(Serial.parseInt());
        }
        else // a number: new target position
        {
            motor.moveAbsAsync(Serial.parseInt());
        }
        while (Serial.available()) Serial.read(); // discard the line end
    }
}
