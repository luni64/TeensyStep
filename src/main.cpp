#include <Arduino.h>
#include "TeensyStep.h"

#define STATUS_PIN PIN_A0

Stepper motor(PIN_A1, PIN_A2);       // STEP pin: 2, DIR pin: 3
StepControl controller;    // Use default settings

bool led_status = false;

void setup(){
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(STATUS_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN, led_status);
    digitalWrite(STATUS_PIN, led_status);
}

void loop(){
    led_status = !led_status;
    digitalWrite(LED_BUILTIN, led_status);
    digitalWrite(STATUS_PIN, led_status);
    motor.setTargetRel(1);  // Set target position to 1000 steps from current position
    controller.move(motor);    // Do the move
}