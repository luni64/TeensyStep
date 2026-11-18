/*==========================================================================
 * 08 Homing: find a reference switch and define the position there
 *
 * The motor runs slowly towards the reference switch on pin 8. The switch
 * triggers an interrupt that stops the motor at once (emergencyStop() does
 * not decelerate; it may be called from an interrupt). The position of the
 * switch is then defined as 0.
 *
 * emergencyStop() may lose steps at high speed because the motor cannot
 * stop instantly. For homing, approach the switch slowly.
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
