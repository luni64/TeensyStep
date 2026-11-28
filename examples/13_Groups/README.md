# Groups

Several motors move synchronized: they start together, arrive together and stay on a straight line
between start and target.

**What you learn**

- How a group is defined: `StepperGroup xy{x, y};`
- The ways to give the targets: in one call (`moveAbs({4'000, 1'000})`, `moveRel()`), or on the motors
  (`setTargetAbs()`, `setTargetRel()`) followed by `move()`.
- A group can be created where it is needed: `StepperGroup{y, z}.moveAbs({0, 0});`
- A group is as fast as its motors allow; `overrideSpeed(0.25)` runs it at a quarter of that speed.
- `stop()` of a group decelerates all members together; they stay synchronized.
- A motor of a group can still be moved on its own.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** three stepper drivers, STEP and DIR on pins 2/3, 4/5 and 6/7.

**Manual:** [Groups](https://luni64.github.io/TeensyStep/groups/)
