# Errors and other functions

## errorCode

```cpp
enum class errorCode; // namespace TeensyStep
```

The result of a command. See [Error handling](../error-handling.md).

| Code | Value | |
|---|---|---|
| `OK` | 0 | success |
| `speedLimited` | −1000 | warning: speed outside the supported range, above the maximum speed, or above what the members of a group allow; the limit is used |
| `accelerationLimited` | −1001 | warning: acceleration outside the supported range, or above what the members of a group allow; the limit is used |
| `targetSpeedNotReached` | −1002 | warning: the way of a group is too short for the speed at its targets; it arrives as close to the speed as it can |
| `invalidArgument` | 1000 | error: invalid argument, for example the wrong number of targets for a group |
| `motorBusy` | 1001 | error: the motor is moving in another group |
| `noTimer` | 1002 | error: no free hardware timer for this move |
| `notSupported` | 1003 | error: not possible in this state, for example other targets for a group that is on its way |
| `internal` | 1999 | error: inconsistent state inside the library |

### isWarning(), isError()

```cpp
constexpr bool isWarning(errorCode c);
constexpr bool isError(errorCode c);
```

`isWarning()` is `true` for the negative codes: the command was executed with a restriction. `isError()`
is `true` for the positive codes: the command was not executed.

### describe()

```cpp
const char* describe(errorCode c);
```

Returns a short text for a code, as the `ErrorHandler` prints it.

---

## attachErrFunc()

```cpp
void attachErrFunc(void (*handler)(errorCode));
void attachErrFunc(const ErrorHandler& handler);
```

Attaches a function that is called for every warning and error of the library, or the ready-made
`ErrorHandler`. `nullptr` removes the function. The function is called in the context of the command
that caused the warning or error, which can be an interrupt.

```cpp
void onError(errorCode code)
{
    Serial.printf("TeensyStep: %d %s\n", (int)code, describe(code));
}

attachErrFunc(onError);
```

## ErrorHandler

```cpp
class ErrorHandler; // namespace TeensyStep

ErrorHandler(Stream& stream);
```

The ready-made error function:

```cpp
attachErrFunc(ErrorHandler(Serial));
```

Warnings are printed to `stream` as `W-<code>: <text>`, and the sketch continues. Errors are printed as
`E-<code>: <text>`; then the handler stops all motors at once and blinks `LED_BUILTIN` in an endless loop.
The stream has to stay valid.

---

## emergencyStopAll()

```cpp
void emergencyStopAll();
```

Stops all motors at once, without deceleration: single motors and groups.

---

## Compile-time settings

| Macro | |
|---|---|
| `TEENSYSTEP_NO_GLOBAL_NAMES` | define before `#include "TeensyStep.h"` to switch off the global names `Stepper`, `StepControl` and `RotateControl` that version 2 sketches use, for example if another library has a class named `Stepper` |
| `TEENSYSTEP_MAX_CHANNELS` | number of moves that can run at the same time; default 16. To be defined for the whole build (compiler option), not in the sketch |
