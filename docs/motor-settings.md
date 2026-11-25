# Motor settings

Besides the maximum speed and the acceleration, a motor has settings that adapt the library to the motor
and its driver. All setters return the motor, so they can be chained:

```cpp
--8<-- "11_MotorSettings/11_MotorSettings.ino"
```

## Motion settings

These settings can be changed at any time, also during a move.

| Function | Default | |
|---|---|---|
| `setMaxSpeed(v)` | 800 steps/s | the highest speed the motor reaches |
| `setAcceleration(a)` | 2500 steps/s² | how fast the speed rises; also used for decelerating, until `setDeceleration()` is called |
| `setDeceleration(a)` | as the acceleration | how fast the speed falls |
| `setPullInSpeed(v)` | 100 steps/s | the speed up to which the motor starts and stops without a ramp |

**Pull-in speed.** A stepper can start directly at a low speed without losing steps. The motor starts at
the pull-in speed and accelerates from there, and it stops from the pull-in speed without a ramp. A higher
value shortens the moves; a value that is too high makes the motor lose steps at the start. Moves at
speeds below the pull-in speed are made without a ramp.

Values outside the supported range are limited and reported as a warning, see
[Error handling](error-handling.md). The range is 0.001 to 1,000,000 steps/s for speeds and 0.001 to
1,000,000,000 steps/s² for accelerations; the step rate a board can really produce is lower, see
[Performance](advanced/performance.md).

## Hardware settings

These settings describe the driver. Set them once in `setup()`, while the motor stands still; during a
move they are refused, and the error `notSupported` is reported to the
[error function](error-handling.md#an-error-function).

| Function | Default | |
|---|---|---|
| `setPulseWidth(us)` | 5 µs | width of the step pulse in microseconds; see the data sheet of the driver |
| `setStepPinPolarity(level)` | `HIGH` | `HIGH`: the step pulse is a positive pulse; `LOW`: a negative pulse |
| `setInverseRotation(inverse)` | `false` | `true` swaps the direction of the motor |
| `setDirSetupTime(us)` | 5 µs | the time the driver needs between a change of DIR and the next step pulse (no effect in this version, see [Limits](advanced/limits.md#timing-of-the-signals)) |
| `setEnablePin(pin, ...)` | none | the enable input of the driver, see below |
| `setEnableFunction(function, ...)` | none | a function that switches the driver, for an enable signal that is not on a pin, see below |

The step pulse also limits the step rate: the pulse and a short pause after it have to fit into the step
period. With the default width of 5 µs the limit is about 170,000 steps/s; for higher rates set a
shorter pulse, if the driver allows it.

## The enable pin

Most drivers have an enable input that switches the motor current on and off. If it is connected, the
library can switch the driver on before a move and off after it:

```cpp
motor.setEnablePin(6, LOW, 50, 500);
```

| Parameter | Default | |
|---|---|---|
| `pin` | | the pin that is connected to the enable input |
| `activeLevel` | `LOW` | the level that switches the driver on |
| `onDelayMs` | 0 | time between switching on and the first step, in milliseconds |
| `offDelayMs` | 0 | time the driver stays on after a move |

- A move that starts within the off-delay of the previous one starts at once, without the on-delay.
- Several motors can share one enable signal: the signal stays on as long as one of them needs it.
- After an emergency stop the driver stays on for the off-delay.
- While only the driver waits to be switched off, the motor counts as standing: `isMoving()` returns
  `false`.

A motor without the current of its driver has no holding torque and can be moved by its load. Without
`setEnablePin()` the library does not touch the driver, and the driver stays on.

### An enable signal that is not on a pin

If the enable input of the driver is not connected to a pin of the board, but for example to a port
extender, give the library a function instead of a pin. The library calls it with `true` where it would
switch the pin on, and with `false` where it would switch it off:

```cpp
void enableDriver(bool on)
{
    extender.digitalWrite(3, on ? LOW : HIGH); // whatever switches the driver
}

motor.setEnableFunction(enableDriver, 50, 500); // on 50 ms before a move, off 500 ms after it
```

The two delays and the rules above are the same as for a pin. Motors that are given the same function
share one signal: the function is called with `true` when the first of them starts and with `false`
when the last of them is done. A motor has either an enable pin or an enable function: the call made
last counts, and `setEnableFunction(nullptr)` removes the function.

!!! warning "The function runs in an interrupt"
    The library calls the function from its step interrupt or with the interrupts disabled. It must not
    wait: no `delay()`, and nothing that needs an interrupt to finish. While it runs, no motor gets a
    step, so a transfer that takes 100 µs delays the steps of all moving motors by that time. If that
    is too long, let the function only note the new state, write it to the extender in `loop()`, and
    choose the on-delay long enough for that.
