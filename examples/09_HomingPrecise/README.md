# HomingPrecise

A reference that does not depend on how the switch is hit: the motor runs to the switch and decelerates
normally, then runs slowly out of it again. The place where the switch opens is the reference; its
interrupt sets the position counter while the motor runs.

**What you learn**

- The three steps of a precise reference: into the switch, slowly out of it (always in the same
  direction and at the same speed), position set where the switch opens.
- `setPositionCounter()` can be called while the motor moves, also from an interrupt.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3, a reference
switch on pin 8 that is LOW while it is active. A switch the mechanics can move through, such as a light
barrier, is better than one they run into.

**What you see:** the motor runs into the switch at 5000 steps/s, stops, and leaves it at 200 steps/s.

**Manual:** [Moving a motor – Homing](https://luni64.github.io/TeensyStep/moving-a-motor/#homing)
