# Stepper

```cpp
class Stepper; // namespace TeensyStep
```

A stepper motor on a driver with STEP and DIR inputs. A `Stepper` cannot be copied; pass it by reference.

---

### Stepper()

```cpp
Stepper(int stepPin, int dirPin);
```

Creates a motor and sets its pins to outputs at their idle levels. No timer is taken: that happens at
the start of each move.

| Parameter | |
|---|---|
| `stepPin` | the pin connected to STEP of the driver |
| `dirPin` | the pin connected to DIR of the driver; 255 for a motor without a DIR signal |

When a `Stepper` object is destroyed while its motor moves, the motor is stopped at once.

---

## Motion settings

Chainable, and allowed at any time. A value outside the supported range is limited, and the warning
`speedLimited` or `accelerationLimited` is reported to the error function.

### setMaxSpeed()

```cpp
Stepper& setMaxSpeed(float stepsPerSec);
```

The highest speed the motor reaches. Default 800 steps/s. During a move the motor accelerates or
decelerates to the new value.

### setAcceleration()

```cpp
Stepper& setAcceleration(float stepsPerSec2);
```

How fast the speed changes. Default 2500 steps/s². The value is also used for decelerating, until
`setDeceleration()` is called.

### setDeceleration()

```cpp
Stepper& setDeceleration(float stepsPerSec2);
```

A separate value for decelerating.

### setPullInSpeed()

```cpp
Stepper& setPullInSpeed(float stepsPerSec);
```

The speed up to which the motor starts, stops and reverses without a ramp. Default 100 steps/s.

---

## Hardware settings

Chainable. To be set while the motor stands still; during a move they are refused, and `notSupported` is
reported to the error function. See [Motor settings](../motor-settings.md#hardware-settings).

### setPulseWidth()

```cpp
Stepper& setPulseWidth(float microseconds);
```

Width of the step pulse. Default 5 µs.

### setStepPinPolarity()

```cpp
Stepper& setStepPinPolarity(int activeLevel);
```

`HIGH` (default): the step pulse is a positive pulse. `LOW`: a negative pulse.

### setInverseRotation()

```cpp
Stepper& setInverseRotation(bool inverse);
```

`true` swaps the direction of the motor: the level of DIR for a rising position.

### setDirSetupTime()

```cpp
Stepper& setDirSetupTime(float microseconds);
```

Time between a change of DIR and the next step pulse. Default 5 µs. No effect in this version, see
[Limits](../advanced/limits.md#timing-of-the-signals).

### setEnablePin()

```cpp
Stepper& setEnablePin(int pin, int activeLevel = LOW, unsigned onDelayMs = 0, unsigned offDelayMs = 0);
```

The enable input of the driver. The library switches the driver on before a move and off after it.
Several motors can be given the same pin, for drivers that share one enable signal: the signal stays on
as long as one of these motors needs it. See [The enable pin](../motor-settings.md#the-enable-pin).

| Parameter | |
|---|---|
| `pin` | the pin connected to the enable input |
| `activeLevel` | the level that switches the driver on |
| `onDelayMs` | time between switching on and the first step, milliseconds |
| `offDelayMs` | time the driver stays on after a move; a new move within it starts at once |

---

## Commands

All commands return `errorCode::OK`, a warning (the command was executed with a restriction) or an error
(the command was not executed and the motor is unchanged). Possible errors: `noTimer`, `motorBusy`.

A command can be given at any time. During a move it replaces the current command without a stop.

### moveAbsAsync(), moveRelAsync()

```cpp
errorCode moveAbsAsync(int32_t pos, float speedAtTarget = 0);
errorCode moveRelAsync(int32_t delta, float speedAtTarget = 0);
```

Starts a move to the position `pos`, or to `delta` steps from the current position, and returns at once.

| Parameter | |
|---|---|
| `pos`, `delta` | the target, in steps |
| `speedAtTarget` | 0 (default): stop at the target. Otherwise: pass the target at this speed and keep running at it. The sign is the direction of passing. See [Targets with a speed](../targets-with-speed.md) |

A speed at the target above the maximum speed is limited to it (warning `speedLimited`).

### moveAbs(), moveRel()

```cpp
errorCode moveAbs(int32_t pos, float speedAtTarget = 0);
errorCode moveRel(int32_t delta, float speedAtTarget = 0);
```

Like the `Async` forms, but they return when the motor stands still at the target or, with a speed at the
target, when it passes the target. Not for use in interrupts.

### rotateAsync()

```cpp
errorCode rotateAsync(float stepsPerSec);
```

Runs the motor at this speed until the next command. The sign is the direction; 0 stops the motor. A
speed above the maximum speed is limited to it (warning `speedLimited`).

### stopAsync(), stop()

```cpp
errorCode stopAsync();
errorCode stop();
```

Decelerates the motor to a standstill. `stop()` returns when the motor stands still. For a motor that
moves in a group, the whole group is stopped.

### emergencyStop()

```cpp
void emergencyStop();
```

Stops the step pulses at once, without deceleration. The position matches the pulses that were made. For
a motor that moves in a group, the whole group is stopped.

---

## Position counter

The library counts every step it makes: up in one direction, down in the other.

### getPosition()

```cpp
int32_t getPosition() const;
```

The position in steps.

### setPositionCounter()

```cpp
void setPositionCounter(int32_t pos);
```

Sets the position counter to `pos`, for example to 0 at the reference position of a machine. The motor
does not move, and the function does not set a target: to move the motor to a position use `moveAbs()`.

The function was called `setPosition()` before. The old name still works; the compiler prints a hint
that it is deprecated.

---

## State

### getSpeed()

```cpp
float getSpeed() const;
```

The current speed in steps/s, with its sign; 0 when the motor stands still.

### isMoving()

```cpp
bool isMoving() const;
```

`true` while the motor moves, on its own or in a group.

---

## Arrival function

### onArrival()

```cpp
void onArrival(void (*callback)());
void onArrival(void (*callback)(Stepper& motor));
```

Sets a function that is called when the motor reaches a target. It is called from an interrupt and may
give the next command. The function has no parameter, or it has one and gets the motor that has
arrived; the second form lets one function serve several motors. `nullptr` removes the function. See
[The arrival function](../targets-with-speed.md#the-arrival-function).

---

## Targets for group moves

### setTargetAbs(), setTargetRel()

```cpp
void setTargetAbs(int32_t pos);
void setTargetRel(int32_t delta);
```

Sets the target of the motor for the next `move()` of a group: a position, or a distance from the current
position. The motor does not move. See [Moving a group](../groups.md#moving-a-group).
