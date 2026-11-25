/*==========================================================================
 * 12 EnableFunction: an enable signal that is not on a pin of the board
 *
 * The enable input of the driver is connected to a port extender (MCP23017,
 * I2C address 0x20, pin GPA0, active low). Instead of a pin, the library
 * gets a function that switches the driver.
 *
 * The library calls the function from an interrupt: it must not wait, and
 * no motor gets a step while it runs (here about 0.3 ms for the transfer).
 *==========================================================================*/

#include "TeensyStep.h"
#include "Wire.h"
using namespace TeensyStep;

Stepper motor(2, 3);

void writeExtender(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(0x20);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void enableDriver(bool on) // called by the library
{
    writeExtender(0x12, on ? 0x00 : 0x01); // register GPIOA: GPA0 low switches the driver on
}

void setup()
{
    Wire.begin();              // 100 kHz, the default
    writeExtender(0x12, 0x01); // GPA0 high: the driver is off
    writeExtender(0x00, 0xFE); // register IODIRA: GPA0 is an output

    motor
        .setEnable(enableDriver, 50, 500) // on 50 ms before a move, off 500 ms after it
        .setMaxSpeed(10'000)              // steps/s
        .setAcceleration(40'000);         // steps/s^2
}

void loop()
{
    motor.moveRel(20'000);  // the driver is switched on, the motor starts 50 ms later
    delay(200);
    motor.moveRel(-20'000); // within the 500 ms: the driver is still on, the motor starts at once
    delay(2'000);           // the driver is switched off 500 ms after the move
}
