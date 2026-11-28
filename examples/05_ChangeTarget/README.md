# ChangeTarget

The motor goes to the position that is typed into the serial monitor, also while it is still on its way
to the one before. `v 5000` sets a new maximum speed.

**What you learn**

- A move command can be given at any time: the motor changes to the new target without stopping first,
  and reverses if the target is behind it.
- `setMaxSpeed()` takes effect at once, also during a move.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3.

**Try it:** type `50000`, and while the motor runs `10000` or `v 2000`.

**Manual:** [Moving a motor – Changes during a move](https://luni64.github.io/TeensyStep/moving-a-motor/#changes-during-a-move)
