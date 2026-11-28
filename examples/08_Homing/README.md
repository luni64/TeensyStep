# Homing

The motor runs slowly towards a reference switch. The switch triggers an interrupt that stops the motor at
once; that place becomes position 0.

**What you learn**

- `emergencyStop()` stops the step pulses at once and can be called from an interrupt.
- `setPositionCounter(0)` gives the place where the motor stands the number 0. The motor does not move.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3, a reference
switch on pin 8 that pulls the pin to GND when it is reached.

**What you see:** the motor runs to the switch, stops, and moves 200 steps off it.

The example 09_HomingPrecise shows a more precise way to find the reference.

**Manual:** [Moving a motor – Homing](https://luni64.github.io/TeensyStep/moving-a-motor/#homing)
