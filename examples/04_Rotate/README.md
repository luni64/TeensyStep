# Rotate

A motor runs at a speed that a potentiometer sets, between -10,000 and +10,000 steps/s. A button stops it.

**What you learn**

- `rotateAsync(speed)` runs the motor at a speed in steps/s; the sign is the direction.
- A further call changes the speed while the motor runs; the motor accelerates, decelerates or reverses.
- `stopAsync()` decelerates to a standstill.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3, a potentiometer
on A0 (between 3.3 V and GND), a button from pin 6 to GND.

**What you see:** the motor follows the potentiometer, through standstill into the other direction, and
stops while the button is pressed.

**Manual:** [Moving a motor – Running at a speed](https://luni64.github.io/TeensyStep/moving-a-motor/#running-at-a-speed)
