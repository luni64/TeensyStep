# CoreXY

An example of the advanced functions (`TeensyStepAdvanced.h`). On a CoreXY machine two motors move the
head together through one belt:

```
motor A = x + y        motor B = x - y
```

A move along x turns both motors in the same direction, a move along y in opposite directions, and on a
diagonal only one of them turns. The path is given in x and y, in mm. The planner in `Planner.h` (the
same file as in the example PathPlanner) gets the formulas above as its kinematics; everything else is
as on a machine with one motor for each axis.

**What you learn**

- That another machine is another kinematics for the planner and nothing else:
  `planner::Kinematics::coreXY(80, 80)`.
- `overrideSpeed(factor)` of the path runs it at a fraction of the planned speeds, also while it runs.
- `overrideSpeed(0)` holds the path: the head brakes along the path and stands on it until another
  factor is set.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** two stepper drivers, motor A with STEP and DIR on pins 2/3, motor B on
pins 4/5; 80 steps of a motor per mm.

**Try it:** while the path runs, type into the serial monitor:

| Key | |
|---|---|
| `-` | slower by 10 % |
| `+` | faster by 10 % |
| `h` | hold: brake and stand on the path |
| `g` | go on |

**Manual:** [Writing a motion planner – Another machine: CoreXY](https://luni64.github.io/TeensyStep/advanced/motion-planner/#another-machine-corexy)
