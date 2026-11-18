[![PlatformIO Registry](https://badges.registry.platformio.org/packages/luni64/library/TeensyStep.svg)](https://registry.platformio.org/libraries/luni64/TeensyStep)
[![arduino-library-badge](https://www.ardu-badge.com/badge/TeensyStep.svg?)](https://www.ardu-badge.com/TeensyStep)
[![GitHub release](https://img.shields.io/github/v/release/luni64/TeensyStep)](https://github.com/luni64/TeensyStep/releases/latest)
[![Documentation](https://github.com/luni64/TeensyStep/actions/workflows/docs.yml/badge.svg?branch=develop)](https://luni64.github.io/TeensyStep/)
[![License: MIT](https://img.shields.io/github/license/luni64/TeensyStep)](LICENCE)

# TeensyStep

Stepper motor library for PJRC Teensy boards, for drivers with STEP and DIR inputs.

**Version 3 is a beta (3.0.0-beta.1): the interface can still change in details until version 3.0.0.** It
is a rewrite and runs on Teensy 4.0, 4.1 and MicroMod and on Teensy 3.5 and 3.6. Its manual is at
[luni64.github.io/TeensyStep](https://luni64.github.io/TeensyStep/). The last release of version 2 is 2.3.4
on the branch [`master`](https://github.com/luni64/TeensyStep/tree/master); the sources of its web site are
in the branch [`gh-pages`](https://github.com/luni64/TeensyStep/tree/gh-pages).

## A first sketch

```cpp
#include "TeensyStep.h"
using namespace TeensyStep;

Stepper motor(2, 3); // STEP pin 2, DIR pin 3

void setup()
{
    motor.setMaxSpeed(20000);      // steps/s
    motor.setAcceleration(50000);  // steps/s^2
}

void loop()
{
    motor.moveRel(1000);           // returns when the motor stands still
    delay(500);
    motor.moveRel(-1000);
    delay(500);
}
```

The commands are member functions of the motor. There are no controller objects, and the library takes and
releases the hardware timers itself. The sketches in [`examples/`](examples/) show the rest, from the
smallest sketch to complete applications; [`examples/README.md`](examples/README.md) lists them.

## What the new version does

- **Commands on the motor:** `moveAbs`, `moveRel`, `rotateAsync`, `stop`, `emergencyStop`. Every move command
  exists in a blocking form (`moveAbs`) and a non-blocking form (`moveAbsAsync`).
- **Changes during a move:** a new target, a new maximum speed or a new acceleration can be given at any
  time. The ramp is calculated from the current state of the motor, not planned once at the start.
- **Target with a speed:** `motor.moveAbs(position, speed)` reaches the position exactly at that speed and
  runs on. A function of the sketch can be called on arrival and can give the next command, which follows
  without a stop.
- **Groups:** `StepperGroup` moves motors synchronized. The motors stay within half a step of the straight
  line between start and target, also while accelerating, decelerating and when the group is stopped early.
  A group can have a speed and an acceleration of its own, and it can rotate with a fixed speed ratio.
- **For motion planners:** `TeensyStepAdvanced.h` adds moves of a group that follow each other without a
  stop (`advanced::GroupPath`). It is kept apart from the standard interface, which stays small.
- **Units:** speeds in steps/s, accelerations in steps/s². Speeds below 1 step/s are possible.
- **Motor settings:** pull-in speed, separate deceleration, pulse width, step polarity, inverse rotation,
  DIR setup time, enable pin with on and off delays.
- **Errors:** every command returns an `errorCode`. `attachErrFunc()` registers a function for warnings and
  errors; `ErrorHandler(Serial)` prints them.
- **Sketches for version 2** compile unchanged: `StepControl` and `RotateControl` are kept as deprecated
  wrappers (see [`examples/version2/`](examples/version2/) and `examples/10_LegacyV2`).

## Boards

| Board | State |
|---|---|
| Teensy 4.0, 4.1, MicroMod | Works. Up to 16 motors or groups move independently at the same time, each on one channel of the QuadTimers (TMR1 to TMR4). A timer that makes PWM or that another library uses is not taken. |
| Teensy 3.5, 3.6 | Works (tested on a Teensy 3.6). Up to 16 motors or groups at the same time, each on one channel of the FlexTimers (FTM0 to FTM3). PWM on the other pins keeps working, and the interval timers stay free for the sketch. |
| Teensy 3.0, 3.1, 3.2, LC | Not supported by the new version: the library calculates in floating point, and these boards have no floating point unit. The library compiles, and every move command returns `noTimer`. Use version 2.3.4. |
| STM32 | Not supported by the new version. Use version 2.3.x. |

## Layout

| Folder | Content |
|---|---|
| `src/api/` | The classes a sketch uses: `Stepper`, `StepperGroup`, `ErrorHandler`; `GroupPath.h`: the advanced functions; `Legacy.h`: the controllers of version 2 |
| `src/core/` | Motion core: `Axis` (one motor), `Group` (motors moving synchronized), `errorCode`. No hardware access; it compiles on a PC and on the boards |
| `src/backend/` | `Backend.h`: the interface to the hardware. `teensy4/`: Teensy 4.0, 4.1, MicroMod. `teensy3/`: Teensy 3.5, 3.6. `none/`: placeholder for boards without a backend |
| `examples/` | Example sketches; `examples/advanced/`: the advanced functions; `examples/version2/`: unchanged examples of version 2 |
| `docs/`, `mkdocs.yml` | The sources of the manual, for MkDocs Material; a push to `develop` deploys it. Preview: `python -m mkdocs serve` in this folder; check: `python -m mkdocs build --strict` |

The API is documented in the headers: `src/api/Stepper.h`, `src/api/StepperGroup.h` and
`src/core/errorCode.h`.
