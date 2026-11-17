/*----------------------------------------------------------
This example demonstrates how to use the serial interface and
digital IO while the controller moves a motor in
the background.

The following serial commands are implemented
   m: move motor
   s: start stop sequence
   e: emergency stop
   h: help

Additionally a pin can be used to stop the motor

------------------------------------------------------------*/
#include "TeensyStep.h"

// stepper and  controller
constexpr int stpPin = 0, dirPin = 1;
Stepper motor(stpPin, dirPin);
StepControl controller;

// pin to stop the motor, connect a push button to this pin
constexpr int stopPin = 2;

// stopwatches 
elapsedMillis displayStopwatch = 0;  // timing the display of the current position
elapsedMillis blinkStopwatch = 0;    // timing the heartbeat LED
elapsedMillis debounceTimer = 0;     // debouncing input pins

int lastPos = 0;

void handlePins();
void handleCommands();

void setup()
{
    while (!Serial);
    Serial.println("Simple Serial Stepper Example");    
    Serial.println("(type h for help)");
    

    motor.setMaxSpeed(5000);
    motor.setAcceleration(50000);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(stopPin, INPUT_PULLUP);  // touch the pin with GND to stop the motor
}


void loop()
{
    // handle incomming commands on the serial interface ------------------
    handleCommands();

    // handle input from pins ---------------------------------------------
    handlePins();

    // display the current motor position every 20ms ----------------------
    if (displayStopwatch > 20)
    {
        displayStopwatch = 0;

        int currentPos = motor.getPosition();
        if (currentPos != lastPos)  // only display if it changed
        {
            lastPos = currentPos;
            Serial.println(currentPos);
        }
    }

    // the usual heartbeat ------------------------------------------------
    if (blinkStopwatch > 250)
    {
        blinkStopwatch = 0;
        digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN)); // toggle LED
    }
}


//------------------------------------------------
// Very simple command interface on USB-Serial:
// m : starts the motor
// s : starts stop sequence
// e : emergency stop

void handleCommands()
{
    if (Serial.available() > 0)                 // skip if the serial buffer is empty
    {
        char cmd = Serial.read();               // get one char from the buffer...
        switch (cmd)                            // ... and analyze it
        {
        case 'm':                               // move command
            if (!controller.isRunning())        // skip move command if motor is running already
            {
                motor.setTargetRel(20000);
                controller.moveAsync(motor);
                Serial.println("Started motor movement");
            }
            else
            {
                Serial.println("Ignored, motor is already running");
            }
            break;

        case 's':                               // stop command
            controller.stopAsync();             // initiate stopping procedure
            Serial.println("Stopping motor");
            break;

        case 'e':                               // emergency stop command
            controller.emergencyStop();
            Serial.println("Emergency Stop");
            break;

        case 'h':                               // help / usage command
        case 'u':
            Serial.println("\nUsage:");
            Serial.println("  m: move motor");
            Serial.println("  s: start stop sequence");
            Serial.println("  e: emergency stop");
            Serial.println("  h: display this help");            
            break;

        default:
            break;
        }
    }
}


//------------------------------------------------
// Connect a pushbutton to the stop pin. 
// Stop command will be issued when pin is 
// pulled to GND.
// Only very simple debouncing implemented. Use "debounce2.h" or 
// similar for a real application

void handlePins()
{    
    if (controller.isRunning() && !digitalReadFast(stopPin) && debounceTimer > 200)
    {
        debounceTimer = 0;                   
        controller.stopAsync();              // initiate stopping procedure
        Serial.println("Stopping motor");
    }
}
