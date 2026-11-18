/*==========================================================================
 * 04 Rotate: run at a speed and change the speed while running
 *
 * A potentiometer on A0 sets the speed between -10000 and +10000 steps/s.
 * A button on pin 6 (to GND) stops the motor.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3);

constexpr int potPin    = A0;
constexpr int buttonPin = 6;

void setup()
{
    pinMode(buttonPin, INPUT_PULLUP);
    motor.setAcceleration(40'000);
}

elapsedMillis timer;

void loop()
{
    if (timer >= 20) // read the potentiometer 50 times per second
    {
        timer = 0;

        if (digitalReadFast(buttonPin) == LOW)
        {
            motor.stopAsync(); // decelerate to a standstill
        }
        else
        {
            float speed = map(analogRead(potPin), 0, 1023, -10'000, 10'000);
            motor.rotateAsync(speed); // new speed in steps/s; fractional values such as 0.5 are allowed
        }
    }
}
