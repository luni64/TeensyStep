/*==========================================================================
 * 09 HomingPrecise: the reference position is where the switch opens
 *
 * Reference switch on pin 8, LOW while it is active. Best is a switch the
 * mechanics can move through (for example a light barrier), not one they
 * run into.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

constexpr int switchPin = 8;

void onSwitchOpens() // interrupt: the motor has left the switch
{
    motor.setPositionCounter(0); // this place is the reference position
    detachInterrupt(switchPin);
}

void home()
{
    // 1. towards the switch; decelerate when it is reached
    if (digitalReadFast(switchPin) == HIGH)
    {
        motor.rotateAsync(-5'000);
        while (digitalReadFast(switchPin) == HIGH) {}
        motor.stop();
    }

    // 2. slowly out of the switch, always in the same direction and at the same speed
    attachInterrupt(switchPin, onSwitchOpens, RISING);
    motor.rotateAsync(200);
    while (digitalReadFast(switchPin) == LOW) {}
    motor.stop();
}

void setup()
{
    pinMode(switchPin, INPUT_PULLUP);
    motor.setMaxSpeed(10'000).setAcceleration(20'000);

    home();
}

void loop()
{
    motor.moveAbs(20'000);
    delay(500);
    motor.moveAbs(200);
    delay(500);
}
