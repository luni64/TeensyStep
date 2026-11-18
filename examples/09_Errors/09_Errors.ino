/*==========================================================================
 * 09 Errors: find out when something does not work
 *
 * No command fails silently. Every command returns an errorCode:
 *   errorCode::OK   success
 *   warning         the command was executed with a restriction, e.g. a
 *                   speed above the supported range was limited
 *   error           the command was not executed; the motor is unchanged
 *
 * Typical errors:
 *   - no free hardware timer (too many motors moving at the same time)
 *   - a group is started while one of its motors is moving in another group
 *
 * Three ways to react (the same as in TeensyTimerTool):
 *   a) check the returned code
 *   b) attach a function that is called for every warning and error
 *   c) attach the built-in handler: attachErrFunc(ErrorHandler(Serial));
 *      It prints warnings and errors. At an error it stops all motors and
 *      halts the sketch with a blinking LED.
 *==========================================================================*/

#include "TeensyStep.h"
using namespace TeensyStep;

Stepper x(2, 3);
Stepper y(4, 5);
Stepper z(6, 7);

StepperGroup xy{x, y};
StepperGroup yz{y, z};

void onError(errorCode err) // b) called for every warning and error
{
    Serial.printf("TeensyStep reported %d\n", (int)err);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    attachErrFunc(onError);

    xy.moveAbsAsync({10'000, 5'000}); // y is now moving in group xy

    errorCode err = yz.moveAbsAsync({0, 3'000}); // a) y is busy: the command is refused
    if (err == errorCode::motorBusy)
    {
        Serial.println("yz could not start, waiting for xy");
        while (xy.isMoving()) {}
        yz.moveAbsAsync({0, 3'000});
    }
}

void loop()
{
}
