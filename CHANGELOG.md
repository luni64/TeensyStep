# Changelog

All notable changes to TeensyStep are documented in this file. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); the versions correspond to the tagged
[GitHub releases](https://github.com/luni64/TeensyStep/releases). The documentation is at
[luni64.github.io/TeensyStep](https://luni64.github.io/TeensyStep/).

## Unreleased

**Added**

- An arrival function can have the motor as its parameter: `void arrived(Stepper& motor)`. It is called
  with the motor that has arrived, so that one function can serve several motors. Functions without a
  parameter work as before.
- `setEnable()` also takes a function that switches the driver on and off, for an enable signal that is
  not on a pin of the board (a port extender, for example). New example `12_EnableFunction`.
- `advanced::setInterruptPriority()` (`TeensyStepAdvanced.h`) sets the priority of the interrupts in which
  the step pulses are made. The default is unchanged (48).
- `advanced::GroupPath` takes its moves ahead: one runs and one waits, and at the targets of a move the
  group goes on with the move that waits, by itself. The next move no longer has to be given in the
  arrival function. `room()` tells whether a move can be added, `onRoom()` sets a function that is called
  when it can, `movesDone()` counts.
- A move of a `GroupPath` says how long it is in the unit of the sketch (`PathMove::length`, for example
  in mm). Its speeds and its acceleration are then in that unit, whichever motor has the longest way, and
  the speed along the path is kept where two moves meet. A motion planner no longer converts its speeds
  for every move.
- Example `advanced/PathPlanner`: a small motion planner on top of `advanced::GroupPath`. It takes a path
  of points in mm with a feed rate, decides how fast each point can be passed, and runs the path without
  stopping where the motors allow it. The planner is a header of the example, made to be copied.
- `GroupPath::overrideSpeed()` runs a path at a fraction of its speeds, also while it runs. The factor 0
  holds the path: the group brakes along the path, through the moves that follow if need be, and stands
  on it until another factor is set.
  `GroupPath::getSpeed()` returns the speed along the path.

**Changed**

- `advanced::GroupPath`: `add(targets, move)` replaces `moveAbsAsync()` and `moveRelAsync()` with their
  speeds, and `setSpeedAtTarget()`. A sketch for the beta that gave the next move in the arrival function
  of the group now gives it ahead, see the example `advanced/GroupWaypoints` and "Paths of a group" in
  the manual. Speed and acceleration of a move are given with the move; those of the `StepperGroup` are
  not used for the moves of a path.
- A speed or an acceleration of a group that is exactly what its motors allow no longer returns the
  warning `speedLimited` or `accelerationLimited` because of a rounding difference.
- The sign of the speed at a target (`moveAbs(pos, speedAtTarget)`, `moveRel()` and their `Async` forms)
  has a new meaning. A positive speed passes the target in the direction in which the motor comes to it,
  also when the target is below the motor; before, a positive speed always meant "counting up" and made
  the motor reverse for a target below it. A negative speed passes the target in the opposite direction:
  the motor goes past it, turns and comes back through it. Sketches that gave a negative speed for a
  target below the motor have to give it without the sign now; the examples `07_Follower` and
  `15_RoseFunction` do so.
- `setEnablePin()` is now `setEnable()`, with one form for a pin and one for a function. The old name
  remains as deprecated.
- `setPosition()` is now `setPositionCounter()`, because the old name was mistaken for a command that
  moves the motor. The old name still works and is deprecated: the compiler prints a hint.
- The examples are numbered by topic. `13_Follower` is now `07_Follower`, `16_HomingPrecise` is
  `09_HomingPrecise`, `09_Errors` is `10_Errors`, `07_Groups` is `13_Groups`, `12_SyncRotation` is
  `14_SyncRotation`, `14_Winder` is `16_Winder` and `10_LegacyV2` is `17_LegacyV2`. The sketches
  themselves are unchanged.

- Manual: new chapter "Behind the scenes", which explains how the library works inside (the ramp, the
  timers, low step rates, the step pulse, groups and Bresenham, the lists in braces). "Motors and drivers"
  has a section on typical drivers and how to connect them, and on the two kinds of bipolar motors. The
  reference of `StepperGroup` explains the two forms of its commands (values in braces or in an array). The installation and
  "Upgrading from version 2" are now one chapter, "Installation and upgrading", at the end of the manual.

## v3.0.0-beta.1 - 2026-11-18

Version 3.0.0-beta.1, the first beta of the next major version: the library is rewritten. **The interface
can still change in details until version 3.0.0.** The manual at
[luni64.github.io/TeensyStep](https://luni64.github.io/TeensyStep/) now describes version 3; its chapter
"Upgrading from version 2" is the migration guide. Version 3 runs on Teensy 4.0, 4.1, MicroMod, 3.5 and
3.6. Teensy 3.0 to 3.2, LC and STM32 are not supported; version 2.3.4 remains available for these boards.

**New**

- Teensy 4.0, 4.1 and MicroMod are supported. Up to 16 motors or groups move independently at the same time.
- Teensy 3.5 and 3.6: up to 16 motors or groups move independently at the same time (version 2: four
  controllers). The library no longer takes the interval timers, and PWM on the pins of the timers it uses
  keeps working.
- The commands are member functions of the motor and of the group: `moveAbs`, `moveRel`, `rotateAsync`, `stop`,
  `emergencyStop`, each move command in a blocking and a non-blocking (`Async`) form. Controller objects are
  no longer needed.
- The target, the maximum speed and the acceleration of a motor can be changed during a move. For a group,
  the maximum speed and the acceleration can be changed.
- A target can have a speed: the motor reaches the position exactly at that speed and runs on. A function
  of the sketch can be called on arrival and can give the next command, which follows without a stop.
- `StepperGroup` moves motors synchronized. The motors stay within half a step of the straight line between
  start and target in every phase of the move, also when the group is stopped early.
- A group can have a maximum speed and an acceleration of its own (`setMaxSpeed`, `setAcceleration`,
  `setDeceleration`), given for the motor with the longest way. The settings of the motors stay the upper
  limits.
- For motion planners and other programs that calculate a path: `TeensyStepAdvanced.h` with
  `advanced::GroupPath`. Its moves have a speed at the targets, so that a group passes them and goes on
  to the next targets without a stop (example `advanced/GroupWaypoints`). These functions are kept apart
  from the standard interface.
- Separate deceleration (`setDeceleration`), DIR setup time (`setDirSetupTime`), enable pin with on and off
  delays (`setEnablePin`).
- Step rates down to 0.001 steps/s.
- Every command returns an `errorCode`. `attachErrFunc()` registers a function for warnings and errors;
  `ErrorHandler` prints them and stops all motors at an error.
- The library takes its hardware timers itself, at the start of a move, and gives them back at the end. It
  does not take a timer that makes PWM or that another library uses, and reports if none is free.
- The motion core has no hardware access and is tested on a PC (folder `test`).
- New examples, `01_HelloStepper` to `16_HomingPrecise`.

**Changed**

- The classes are in the namespace `TeensyStep`. `Stepper`, `StepControl` and `RotateControl` are also
  available without the namespace, so that sketches for version 2 compile unchanged; define
  `TEENSYSTEP_NO_GLOBAL_NAMES` before the include to switch this off.
- `StepControl` and `RotateControl` are deprecated wrappers on the new commands. The examples of version 2
  are in `examples/version2`.
- `setPullInOutSpeed()` is deprecated: use `setPullInSpeed()`.

**Removed**

- Teensy 3.0, 3.1, 3.2 and LC are not supported by the new version: it calculates in floating point, and
  these boards have no floating point unit. Version 2.3.4 remains available for them.
- STM32 is not supported by the new version. Use version 2.3.x.

**Fixed**

- The Winder example of version 2 also compiles outside the Arduino IDE (missing declaration of
  `printCurrent()`).

**Known issues**

- `setDirSetupTime()` has no effect. The DIR signal changes at the end of the step pulse before the first
  step in the new direction, so the time between the change and that step is the step period minus the
  pulse width. This is long at a reversal of a single motor, but can be short where a group passes a
  waypoint at speed (`advanced::GroupPath`).
- A sketch that asks for more steps per second than the board can make leaves no processor time for
  anything else, including USB. The board then has to be reset with its button. The manual lists the step
  rates the boards reach (Advanced, Performance).
- Motors that move independently delay each other's steps slightly when their steps are due at the same
  moment: about 1 µs with three motors at 50,000 steps/s, about 6 µs with 16 motors. The delay does not
  add up, and a single motor or a single group is not affected.
- The Teensy 3.5 is built but not tested on a board; the Teensy 3.6 is.

**Planned changes until version 3.0.0**

These points are under discussion. They can change the interface in details.

- Groups: the way the speed and the acceleration of a group are given (`StepperGroup::setMaxSpeed()`,
  `setAcceleration()`, `setDeceleration()`) may be simplified.
- `setPosition()` gets a new name; the old name remains as deprecated. (Done after the beta: see
  "Unreleased".)
- `onArrival()` gets a way to serve several motors with one function. (Done after the beta: see
  "Unreleased".)
- `advanced::GroupPath` may be replaced by an interface made for motion planners. (Done after the beta: see
  "Unreleased".)
- `setDirSetupTime()` gets its function.
- The numbers of the examples may change. (Done after the beta: see "Unreleased".)

## v2.3.4 - 2023-04-13

**Fixed**

- Library manifests (library.properties, library.json) corrected so that the Arduino Library Manager and
  PlatformIO pick up the release.

## v2.3.3 - 2023-04-07

**Fixed**

- Compile errors caused by the Arduino core defining `abs`, `max` and similar functions as macros.
- Link errors with some compilers: the static `constexpr` class members now have definitions.

## v2.3.2 - 2022-03-29

**New**

- `RotateControl` accepts a callback, like `StepControl`.

**Fixed**

- Compile errors caused by the Arduino `abs` macro (`std::abs` replaced by `abs`).

## v2.3.1 - 2021-11-22

**Fixed**

- Overflow of the speed calculation for large speed and acceleration values.
- A motor did not restart after `overrideSpeed(0)`.

## v2.3.0 - 2021-09-28

**Changed**

- Synchronized group movement runs at the highest possible speed. Previously the group speed was derived
  from the slowest motor; now it is chosen so that no motor of the group exceeds its maximum speed
  ([discussion #107](https://github.com/luni64/TeensyStep/discussions/107)).

**Fixed**

- Stopping a motor that has no steps left works ([#92](https://github.com/luni64/TeensyStep/issues/92)).
- STM32F4: the first step pulse was too long; faster start of the step timer.

## v2.2.0 - 2020-10-05

**New**

- Support for STM32F4, contributed by ramboerik ([#75](https://github.com/luni64/TeensyStep/pull/75)).
  Requires the STM32 Arduino core 1.9.0 or later. Verified on STM32F4 only. Known issue: the first step
  interrupt fires at a slightly wrong time. Do not use more than 4 `StepControl` instances on STM32.

**Changed**

- `Stepper` member functions are virtual, so that derived classes can override `setTargetAbs()` and
  `setTargetRel()`.
- Platform selection works for all Teensy 3.x boards.

## v2.1.0 - 2020-06-22

**New**

- Error handler.
- Pull-in and pull-out speed settings for `StepControl` (experimental).
- Speed override in the move commands.
- Version information (`version.h`).

**Changed**

- Controllers allocate their hardware timers only while a movement is running and release them in the
  destructor. Controllers can therefore be local objects.
- Library code moved into the namespace `TeensyStep`.

**Fixed**

- A controller using on-demand timers did not restart after `stop()`.
- `RotateControl` did not release its timer.
- `isRunning()` returned `false` too early.
- Overflow in the speed calculation.
- Compile errors with newer compilers (casts to `constexpr`, clash of `std::abs` with the Arduino `abs`).

## V2.0.0 - 2019-05-05

First release of TeensyStep version 2. **The API is new**: sketches written for version 1 must be adapted.
Version 1 is still available in the branch [Version-1](https://github.com/luni64/TeensyStep/tree/Version-1)
(bug fixes only).

**New**

- Separate controllers for positioning (`StepControl`) and continuous rotation (`RotateControl`), with
  speed and acceleration override while moving.
- Custom acceleration profiles: the controllers are templates on an accelerator class
  (`LinStepAccelerator`, `LinRotAccelerator`, `SinRotAccelerator`).
- Callback when a movement is completed.
- Generic tick timer in addition to the hardware timers.
- Coil winder example (examples/Applications/Winder).

**Changed**

- The pulse width and the acceleration update rate are no longer template parameters.

**Fixed**

- A motor occasionally did not stop exactly at its target.
- Double pulse at motor start; errors in `stop()` and emergency stop; problems when controllers were
  destroyed.

## V1.0.10 - 2019-04-29

Last release of version 1, see the branch [Version-1](https://github.com/luni64/TeensyStep/tree/Version-1).
