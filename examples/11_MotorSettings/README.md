# MotorSettings

All settings of a motor in one chain of calls: those that describe the driver and those of the motion.

**What you learn**

| Function | |
|---|---|
| `setPulseWidth(us)` | width of the step pulse, default 5 µs; see the data sheet of the driver |
| `setStepPinPolarity(level)` | `HIGH`: the driver steps on a positive pulse (default); `LOW`: on a negative pulse |
| `setInverseRotation(true)` | swaps the direction of the motor |
| `setDirSetupTime(us)` | time the driver needs between a change of DIR and the next step pulse, 0 to 200 µs, default 5 µs |
| `setEnable(pin, ...)` | optional: the library switches the driver on before a move and off after it |
| `setPullInSpeed(v)` | speed up to which the motor starts and stops without a ramp, default 100 steps/s |
| `setDeceleration(a)` | optional: a value of its own for decelerating |

- Maximum speed, acceleration, deceleration and pull-in speed can be changed at any time, also during a
  move.
- The other settings describe the hardware: set them once in `setup()`, while the motor stands still.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2, DIR on pin 3 and its enable input
on pin 6.

**Manual:** [Motor settings](https://luni64.github.io/TeensyStep/motor-settings/)
