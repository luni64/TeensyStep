# TeensyStep

Stepper motor library for PJRC Teensy boards, for drivers with STEP and DIR inputs.

**This branch (`develop`) holds the next major version of TeensyStep. It is work in progress:** the
interface can still change, and it runs on Teensy 4.x only so far. The released version is 2.3.4 on the
branch [`master`](https://github.com/luni64/TeensyStep/tree/master); its documentation is at
[luni64.github.io/TeensyStep](https://luni64.github.io/TeensyStep/).

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
  A group can also rotate with a fixed speed ratio.
- **Units:** speeds in steps/s, accelerations in steps/s². Speeds below 1 step/s are possible.
- **Motor settings:** pull-in speed, separate deceleration, pulse width, step polarity, inverse rotation,
  DIR setup time, enable pin with on and off delays.
- **Errors:** every command returns an `errorCode`. `attachErrFunc()` registers a function for warnings and
  errors; `ErrorHandler(Serial)` prints them.
- **Sketches for version 2** compile unchanged: `StepControl` and `RotateControl` are kept as deprecated
  wrappers (see [`examples/version2/`](examples/version2/) and `examples/11_LegacyV2`).

## Boards

| Board | State |
|---|---|
| Teensy 4.0, 4.1, MicroMod | Works. Up to 16 motors or groups move independently at the same time, each on one channel of the QuadTimers (TMR1 to TMR4). A timer that makes PWM or that another library uses is not taken. |
| Teensy 3.x, LC | Not yet. The library compiles, and every move command returns `noTimer`. Use version 2.3.4 on these boards. |
| STM32 | Not supported by the new version. Use version 2.3.x. |

## Layout

| Folder | Content |
|---|---|
| `src/api/` | The classes a sketch uses: `Stepper`, `StepperGroup`, `ErrorHandler`; `Legacy.h`: the controllers of version 2 |
| `src/core/` | Motion core: `Axis` (one motor), `Group` (motors moving synchronized), `errorCode`. No hardware access; it compiles on a PC and on the boards |
| `src/backend/` | `Backend.h`: the interface to the hardware. `teensy4/`: Teensy 4.0, 4.1, MicroMod. `host/`: simulated backend for the PC tests. `none/`: placeholder for boards without a backend |
| `examples/` | Example sketches; `examples/version2/`: unchanged examples of version 2 |
| `test/` | Tests that run on a PC: the core directly, and the API on the simulated backend |

The API is documented in the headers: `src/api/Stepper.h`, `src/api/StepperGroup.h` and
`src/core/errorCode.h`.

## PC tests

```
python test/run_tests.py                 # build what changed, run all tests
python test/run_tests.py -tc="ramp*"     # run the test cases whose names match
python test/run_tests.py --clean         # rebuild everything
python test/mutate.py                    # check the tests: deliberate errors in the library, each must be noticed (25 min)
python test/mutate.py group              # only the errors whose description contains this text
```

The tests need Python 3.11 or newer and g++ (on Windows: `winget install BrechtSanders.WinLibs.POSIX.UCRT`).
They are compiled as C++17 with GNU extensions, like the Teensy builds; the core sources additionally
without exceptions and RTTI. Warnings are errors.

The tests are in `test/test_*.cpp`. `test/sim.h` runs a motor, records its steps and checks them against
the limits. `test/doctest/doctest.h` is the test framework (doctest 2.4.12, MIT licence); `test/.build/` is
the build output and is not tracked.
