# NonBlocking

Two motors move independently of each other while `loop()` goes on: it blinks the LED, prints the state of
both motors every 100 ms and sends them back when both have arrived.

**What you learn**

- The `Async` commands return at once; the motor moves in the background.
- How to ask for the state of a motor: `isMoving()`, `getPosition()`, `getSpeed()`.
- That motors with commands of their own move independently, each with its own speed and acceleration.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** two stepper drivers, STEP and DIR on pins 2 and 3, and on pins 4 and 5.

**Output:** every 100 ms a line with the position (steps) and the speed (steps/s) of both motors:

```
m1: pos <position>  speed <speed>   m2: pos <position>  speed <speed>
```

**Manual:** [Moving a motor – Blocking and non-blocking commands](https://luni64.github.io/TeensyStep/moving-a-motor/#blocking-and-non-blocking-commands)
