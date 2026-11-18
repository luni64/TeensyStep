/*==========================================================================
 * 08 Homing: find a reference switch and define the position there
 *
 * Reference switch on pin 8, LOW when it is reached.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

constexpr int switchPin = 8;

void onSwitch() // interrupt: switch reached
{
    motor.emergencyStop();
}

void home()
{
    attachInterrupt(switchPin, onSwitch, FALLING);

    motor.rotateAsync(-500);    // slowly towards the switch
    while (motor.isMoving()) {} // wait until the switch has stopped the motor

    detachInterrupt(switchPin);
    motor.setPosition(0); // this is the reference position
    motor.moveAbs(200);   // move off the switch
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
