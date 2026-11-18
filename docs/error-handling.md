# Error handling

No command of the library fails silently. Every command either works in the current state of the motor,
or it reports why not and leaves the motor unchanged.

## What a command returns

Every command returns an `errorCode`:

| | Value | Meaning |
|---|---|---|
| `errorCode::OK` | 0 | the command was executed |
| a warning | negative | the command was executed with a restriction |
| an error | positive | the command was not executed; the motor is unchanged |

```cpp
errorCode err = motor.moveAbsAsync(5000);
if (err != errorCode::OK)
{
    Serial.printf("moveAbsAsync returned %d\n", (int)err);
}
```

`isWarning(code)` and `isError(code)` tell the two kinds apart, and `describe(code)` returns a short text
for a code.

### Warnings

| Code | Value | Reported when |
|---|---|---|
| `speedLimited` | −1000 | a speed is outside the supported range, a speed at the target is above the maximum speed, or the speed of a group is above what its members allow. The limit is used |
| `accelerationLimited` | −1001 | an acceleration is outside the supported range, or the acceleration of a group is above what its members allow. The limit is used |
| `targetSpeedNotReached` | −1002 | a move of a [`GroupPath`](advanced/group-paths.md) is too short for the speed at its targets. The group arrives as close to the speed as it can |

### Errors

| Code | Value | Reported when |
|---|---|---|
| `invalidArgument` | 1000 | an argument is invalid, for example a group move with the wrong number of targets |
| `motorBusy` | 1001 | the motor is moving in another group, or on its own while a group with it is started |
| `noTimer` | 1002 | no hardware timer is free for this move, see [Timers and interrupts](advanced/timers.md) |
| `notSupported` | 1003 | the command is not possible in this state, for example new targets for a moving group, or a hardware setting during a move |
| `internal` | 1999 | inconsistent state inside the library; please report it |

The values are in a range of their own, so that a printed code identifies the library.

## An error function

Checking every return value is tedious, and the setters return the motor and not a code. `attachErrFunc()`
registers a function of the sketch that is called for every warning and every error:

```cpp
--8<-- "09_Errors/09_Errors.ino"
```

The error function can be called from an interrupt, when the command that caused it was given from one.
`attachErrFunc(nullptr)` removes the function.

## The ready-made error handler

During development the simplest way is the handler that comes with the library:

```cpp
void setup()
{
    Serial.begin(115200);
    attachErrFunc(ErrorHandler(Serial));
    ...
}
```

- A warning is printed as `W-1000: <text>`, and the sketch continues.
- An error is printed as `E-1002: <text>`. Then the handler stops all motors at once and blinks the
  built-in LED in an endless loop: the sketch is halted.

The handler follows the pattern of the one in TeensyTimerTool.
