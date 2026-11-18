/*==========================================================================
 * 03 NonBlocking: start a move and do other things while it runs
 *
 * Two independent motors; loop() blinks the LED and prints their state.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper m1(2, 3);
Stepper m2(4, 5);

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    m1.setMaxSpeed(10'000).setAcceleration(20'000);
    m2.setMaxSpeed(3'000).setAcceleration(5'000);

    m1.moveAbsAsync(50'000); // both calls return immediately,
    m2.moveAbsAsync(-8'000); // the motors run in the background
}

elapsedMillis timer;

void loop()
{
    if (timer >= 100)
    {
        timer = 0;
        digitalToggleFast(LED_BUILTIN);

        Serial.printf("m1: pos %6d  speed %6.0f   m2: pos %6d  speed %6.0f\n",
                      m1.getPosition(), m1.getSpeed(), m2.getPosition(), m2.getSpeed());

        if (!m1.isMoving() && !m2.isMoving()) // both have arrived: send them back
        {
            m1.moveAbsAsync(m1.getPosition() == 0 ? 50'000 : 0);
            m2.moveAbsAsync(m2.getPosition() == 0 ? -8'000 : 0);
        }
    }
}
