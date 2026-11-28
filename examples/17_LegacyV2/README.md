# LegacyV2

A sketch written for TeensyStep 2, unchanged. `StepControl` and `RotateControl` are kept as thin wrappers
around the new functions, so that existing sketches keep working.

**What you learn**

- The controllers are deprecated: the compiler shows a hint, and new sketches give the commands to the
  motors and groups.
- A version 2 sketch needs no `using namespace TeensyStep;`: the header provides the old global names
  `Stepper`, `StepControl` and `RotateControl`. Define `TEENSYSTEP_NO_GLOBAL_NAMES` before the include
  to switch them off, for example if another library also has a class named `Stepper`.
- How the old commands translate:

| Version 2 | New |
|---|---|
| `StepControl c;` | not needed |
| `m.setTargetAbs(1000); c.move(m);` | `m.moveAbs(1000);` |
| `c.moveAsync(m1, m2);` | `StepperGroup{m1, m2}.moveAsync();` |
| `RotateControl r; r.rotateAsync(m);` | `m.rotateAsync(speed);` |
| `r.overrideSpeed(0.5);` | `m.rotateAsync(newSpeed);` |
| `c.isRunning()` | `m.isMoving()` |

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** two stepper drivers, STEP and DIR on pins 2/3 and 9/10.

**Manual:** [Installation and upgrading – Upgrading from version 2](https://luni64.github.io/TeensyStep/installation/#upgrading-from-version-2)
