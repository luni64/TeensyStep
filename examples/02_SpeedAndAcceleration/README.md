# SpeedAndAcceleration

A motor moves between the positions 0 and 40,000 with a maximum speed and an acceleration set by the
sketch, and prints its position after each move.

**What you learn**

- How to set the maximum speed and the acceleration: `setMaxSpeed()`, `setAcceleration()`. The setters
  return the motor, so they can be chained.
- The difference between absolute targets (`moveAbs()`) and relative ones (`moveRel()`).
- How to give the current position a number (`setPositionCounter()`) and how to read it (`getPosition()`).

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3.

**Output:**

```
position: 40000
position: 0
position: 40000
...
```

**Manual:** [Getting started – Speed and acceleration](https://luni64.github.io/TeensyStep/#speed-and-acceleration)
