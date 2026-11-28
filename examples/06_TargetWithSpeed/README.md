# TargetWithSpeed

A motor runs a speed profile along a track: it passes position 10,000 at 2000 steps/s, accelerates,
passes 40,000 at 12,000 steps/s and stops at 50,000.

| Position | 0 | 10,000 | 40,000 | 50,000 |
|---|---|---|---|---|
| Speed (steps/s) | 0 | 2000 | 12,000 | 0 |

**What you learn**

- The second parameter of a move command is the speed at the target: `moveAbsAsync(10'000, 2'000);`
- `onArrival()` sets a function that is called when the motor reaches a target. It gives the next move,
  which follows without a stop.
- The arrival function is called from an interrupt: it has to be short.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3.

**What you see:** one move of 50,000 steps with a slow and a fast section.

**Manual:** [Targets with a speed – The arrival function](https://luni64.github.io/TeensyStep/targets-with-speed/#the-arrival-function)
