# Reference

```cpp
#include "TeensyStep.h"
using namespace TeensyStep;
```

All classes and functions of the library are declared in the namespace `TeensyStep`.

| | |
|---|---|
| [`Stepper`](stepper.md) | a stepper motor on a driver with STEP and DIR inputs |
| [`StepperGroup`](stepper-group.md) | motors that move synchronized |
| [`errorCode`, `attachErrFunc()`, `ErrorHandler`, `emergencyStopAll()`](errors.md) | errors and other functions |
| [`advanced::GroupPath`](../advanced/group-paths.md#reference) | moves of a group that follow each other without a stop; header `TeensyStepAdvanced.h` |

Units: positions in steps, speeds in steps/s, accelerations in steps/s², pulse times in microseconds.

Unless stated otherwise, all functions can be called at any time, also during a move and from
interrupts. Commands that return an `errorCode` report every warning and error to the
[error function](errors.md#attacherrfunc) as well.

The controllers of version 2, `StepControl` and `RotateControl`, are still available but deprecated; see
[Upgrading from version 2](../installation.md#upgrading-from-version-2).
