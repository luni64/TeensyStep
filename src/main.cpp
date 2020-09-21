
#ifdef TEST
#include <Arduino.h>
#include <vector>
#include "TeensyStep.h"

using std::vector;

#define ENABLE_PIN D8

Stepper stepper_x(D2, D5);       // STEP pin: 2, DIR pin: 3
Stepper stepper_y(D3, D6);
StepControl controller;    // Use default settings


class CPoint{
    public:
        CPoint(int x, int y): x(x), y(y) {}
        int x = 0;
        int y = 0;
};

vector<CPoint*> points;

void setup(){
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(ENABLE_PIN, LOW);

    points.push_back(new CPoint(100, 100));
    points.push_back(new CPoint(200, 200));
    points.push_back(new CPoint(300, 300));
    points.push_back(new CPoint(400, 400));
    points.push_back(new CPoint(500, 500));
    points.push_back(new CPoint(600, 600));
    points.push_back(new CPoint(700, 700));
    points.push_back(new CPoint(800, 800));
    points.push_back(new CPoint(900, 900));
    points.push_back(new CPoint(1000, 1000));

    #ifdef TEENSY3
    Serial.println("TEENSY3");
    #else
    Serial.println("STM32F4");
    #endif
}

void loop(){
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Starting new sequence");

    for(auto& point : points){
        Serial.printf("Moving to x: %d, y: %d\r\n", point->x, point->y);
        stepper_x.setTargetAbs(point->x);
        stepper_y.setTargetAbs(point->y);
        controller.moveAsync(stepper_x, stepper_y);
        while(controller.isRunning());
    }

    //controller.step_calls  = controller.pulse_calls = controller.acc_calls = 0;
    //Serial.printf("steps: %u, pulses: %u, acc: %u\r\n", controller.step_calls, controller.pulse_calls, controller.acc_calls);
    digitalWrite(LED_BUILTIN, LOW);
}

#endif
