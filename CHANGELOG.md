# Changelog

All notable changes to TeensyStep are documented in this file. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); the versions correspond to the tagged
[GitHub releases](https://github.com/luni64/TeensyStep/releases). The documentation is at
[luni64.github.io/TeensyStep](https://luni64.github.io/TeensyStep/).

## Unreleased

**Fixed**

- The Winder example also compiles outside the Arduino IDE (missing declaration of `printCurrent()`).

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
