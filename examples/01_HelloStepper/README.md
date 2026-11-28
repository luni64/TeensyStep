# HelloStepper

The smallest sketch: one motor makes 1000 steps forward and 1000 steps back, again and again. No
settings are made, so the motor runs with the defaults (800 steps/s, 2500 steps/s²).

**What you learn**

- How a motor is defined: `Stepper motor(2, 3);` with the STEP and the DIR pin.
- That the commands are given to the motor itself: `motor.moveRel(1000);`
- That `moveRel()` returns when the motor has arrived.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3 (any pins work).

**What you see:** the motor turns to and fro, with a pause of half a second at each end.

**Manual:** [Getting started – A first sketch](https://luni64.github.io/TeensyStep/#a-first-sketch)
