# SyncRotation

Two motors rotate with a fixed speed ratio: the second one always makes 3 steps while the first makes 1,
also while they accelerate, decelerate and reverse.

**What you learn**

- `rotateAsync({5'000, 15'000})` of a group runs its members at these speeds and keeps their ratio exactly.
- `overrideSpeed(factor)` runs the group at a fraction of these speeds; a negative factor reverses all
  members.
- `stop()` decelerates the members together.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** two stepper drivers, STEP and DIR on pins 2/3 and 4/5.

**What you see:** 3 s at 5000 and 15,000 steps/s, 3 s at half of that, 3 s backwards, then a stop.

**Manual:** [Groups – Rotating a group](https://luni64.github.io/TeensyStep/groups/#rotating-a-group)
