
#ifdef TEST
#include <Arduino.h>
#include "TeensyStep.h"

#define STATUS_PIN PIN_A0

Stepper motor(PIN_A1, PIN_A2);       // STEP pin: 2, DIR pin: 3
StepControl controller;    // Use default settings

void setup(){
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(STATUS_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(STATUS_PIN, LOW);

    #ifdef TEENSY3
    Serial.println("TEENSY3");
    #else
    Serial.println("STM32F4");
    #endif
}

void loop(){
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(STATUS_PIN, HIGH);

    motor.setTargetRel(10);  // Set target position to 1000 steps from current position
    controller.move(motor);    // Do the move
    while(controller.isRunning());

    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(STATUS_PIN, LOW);
}

#endif
