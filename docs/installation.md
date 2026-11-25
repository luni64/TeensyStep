# Installation and upgrading

## Installation

The library runs on Teensy 4.0, 4.1 and MicroMod and on Teensy 3.5 and 3.6, with Teensyduino in the
Arduino IDE or with PlatformIO.

=== "Arduino IDE"

    Open the Library Manager (*Tools → Manage Libraries…*), search for TeensyStep and install version
    3.0.0-beta.1 or later. To go back to version 2, select 2.3.4 in the same place.

=== "PlatformIO"

    Add the version to `platformio.ini`:

    ```ini
    lib_deps = https://github.com/luni64/TeensyStep#v3.0.0-beta.1
    ```

=== "git"

    Clone the repository into the libraries folder of the sketchbook and check out the version:

    ```
    git clone -b v3.0.0-beta.1 https://github.com/luni64/TeensyStep
    ```

## Upgrading from version 2

The new version is a rewrite with a different programming model: the commands are given to the motors
and groups, and the controller objects of version 2 are no longer needed. Existing sketches nevertheless
keep working, because the controllers are still there as thin wrappers on the new commands.

!!! warning "Boards"
    The new version runs on Teensy 4.0, 4.1 and MicroMod and on Teensy 3.5 and 3.6. Teensy 3.0 to 3.2,
    LC and STM32 are not supported by the new version (1); version 2.3.4 remains available for them.
    { .annotate }

    1.  **Why not**

        The new version calculates the ramps in floating point at every step. Teensy 3.0 to 3.2 and LC
        have no floating point unit, which makes these calculations too slow there.

### Existing sketches

A sketch written for version 2 compiles without changes:

```cpp
--8<-- "10_LegacyV2/10_LegacyV2.ino"
```

- `StepControl` and `RotateControl` are deprecated: the compiler prints a hint for each of them. New
  sketches should give the commands to the motors and groups.
- The names `Stepper`, `StepControl` and `RotateControl` are available without a namespace, as in
  version 2. If another library has a class named `Stepper`, define `TEENSYSTEP_NO_GLOBAL_NAMES` before
  the include and use the names with `TeensyStep::`.
- The examples of version 2 are in `examples/version2/`, unchanged; they are compiled in the build tests
  of the library.

### Translating a sketch

| Version 2 | New |
|---|---|
| `StepControl controller;` | not needed |
| `m.setTargetAbs(1000); controller.move(m);` | `m.moveAbs(1000);` |
| `m.setTargetRel(200); controller.moveAsync(m);` | `m.moveRelAsync(200);` |
| `controller.move(m1, m2);` | `StepperGroup{m1, m2}.move();` or `StepperGroup{m1, m2}.moveAbs({1000, 500});` |
| `controller.isRunning()` | `m.isMoving()`, `group.isMoving()` |
| `controller.stop()`, `stopAsync()`, `emergencyStop()` | the same functions of the motor or the group |
| `controller.setCallback(f)` | `m.onArrival(f)`, `group.onArrival(f)` |
| `controller.getCurrentSpeed()` | `m.getSpeed()` |
| `RotateControl r; m.setMaxSpeed(v); r.rotateAsync(m);` | `m.rotateAsync(v);` |
| `r.rotateAsync(m1, m2);` | `StepperGroup{m1, m2}.rotateAsync({v1, v2});` |
| `r.overrideSpeed(0.5);` for one motor | `m.rotateAsync(newSpeed);` |
| `r.overrideSpeed(0.5);` for several motors | `group.overrideSpeed(0.5);` |
| `m.setPullInOutSpeed(in, out)` | `m.setPullInSpeed(v)` |

### What is different

- **Speeds are not limited to integers.** Speeds and accelerations are floating point numbers in steps/s
  and steps/s².
- **The direction of a rotation** is the sign of the speed given to `rotateAsync()`. With the version 2
  controllers a negative maximum speed still rotates backwards.
- **Timers.** Version 2 needed a controller per independent move, and each controller reserved its timers
  for as long as it existed. Now every moving motor or group takes a timer for the time of the move. On
  Teensy 4.x these are QuadTimer channels and no longer interval timers, see
  [Timers and interrupts](advanced/timers.md).
- **Changes during a move.** Target, maximum speed and acceleration of a motor can change while it moves.
  In version 2 this was only possible for the speed of a `RotateControl`.
- **Errors.** Commands return an `errorCode` instead of failing silently, see
  [Error handling](error-handling.md). `isOk()` of the controllers always returns `true`.
- **The update period of the acceleration**, the second parameter of the version 2 controllers, has no
  effect: the ramp is calculated at every step.
