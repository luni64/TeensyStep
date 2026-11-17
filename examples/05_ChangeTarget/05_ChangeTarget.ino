/*==========================================================================
 * 05 ChangeTarget: give a new target while the motor is moving
 *
 * A move command may be given at any time. If the motor is already moving,
 * it changes course towards the new target without stopping first. If the
 * new target lies behind it, it decelerates, reverses and runs back.
 *
 * Type a position into the serial monitor; the motor follows the latest
 * value. Changing the maximum speed during a move works the same way
 * (type "v 5000").
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

void setup()
{
    Serial.begin(115200);
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
