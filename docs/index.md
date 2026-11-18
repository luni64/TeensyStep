# Getting started

TeensyStep moves stepper motors that are connected to drivers with STEP and DIR inputs (1). It produces
the step pulses with the acceleration and deceleration ramps a stepper needs, for single motors and for
groups of motors that move synchronized.
{ .annotate }

1.  **STEP and DIR**

    Practically all modern stepper drivers have these two inputs. Every pulse on STEP moves the motor by
    one step (or microstep); the level on DIR sets the direction. See
    [Motors and drivers](motors-and-drivers.md).

The library offers

- commands on the motor itself: `motor.moveAbs(1000)`, `motor.rotateAsync(500)`, `motor.stop()`,
- a new target, speed or acceleration at any time, also while the motor moves,
- targets with a speed, so that a motor can pass a series of points without stopping,
- groups of motors that stay on a straight line between start and target,
- step rates from 0.001 to 300,000 steps per second,
- up to 16 motors or groups that move independently at the same time (Teensy 4.x).

!!! warning "Draft for a development version"
    This manual describes the next major version of TeensyStep, which is under development on the branch
    `develop`. Functions can still change. It runs on **Teensy 4.0, 4.1 and MicroMod** and on
    **Teensy 3.5 and 3.6**. Teensy 3.0 to 3.2 and LC are not supported: the library compiles there, but
    no motor moves; use version 2.3.4 on these boards. Sketches written for version 2
    still compile, see [Upgrading from version 2](upgrading.md).

## Installation

The development version is not in the library managers. Install it from GitHub:

=== "Arduino IDE"

    Download the branch `develop` as
    [zip file](https://github.com/luni64/TeensyStep/archive/refs/heads/develop.zip) and install it with
    *Sketch → Include Library → Add .ZIP Library…*. Remove an installed TeensyStep 2 first.

=== "PlatformIO"

    Add the branch to `platformio.ini`:

    ```ini
    lib_deps = https://github.com/luni64/TeensyStep#develop
    ```

=== "git"

    Clone the repository into the libraries folder of the sketchbook and check out `develop`:

    ```
    git clone -b develop https://github.com/luni64/TeensyStep
    ```

## Connecting a motor

Connect the STEP and DIR inputs of the driver to two pins of the Teensy, and GND of the driver's logic
side to GND of the Teensy. Any digital pins work. A Teensy 4 has 3.3 V outputs; drivers with 5 V or
opto-coupler inputs may need a level shifter, see [Motors and drivers](motors-and-drivers.md#logic-levels).

## A first sketch

The following sketch moves a motor 1000 steps forward and back, forever:

```cpp
--8<-- "01_HelloStepper/01_HelloStepper.ino"
```

The sketch declares a `Stepper` with the numbers of its STEP and DIR pins. `moveRel(1000)` moves the motor
1000 steps from its current position and returns when the motor stands still again. The line
`using namespace TeensyStep;` (1) makes the classes of the library available under their short names.
{ .annotate }

1.  **Namespace**

    All classes of the library are declared in the *namespace* `TeensyStep`; their full names are
    `TeensyStep::Stepper` and so on. This avoids name clashes with other libraries. The line
    `using namespace TeensyStep;` makes the classes available under their short names.
    [More about namespaces](https://www.learncpp.com/cpp-tutorial/using-declarations-and-using-directives/)

Without further settings the motor runs at 800 steps/s and accelerates with 2500 steps/s². These values
are slow, but they work with any normal stepper.

## Speed and acceleration

A stepper cannot start at full speed: it has to be accelerated, or it loses steps (1). The two settings
that matter most are therefore the maximum speed and the acceleration:
{ .annotate }

1.  **Lost steps**

    The driver has no feedback from the motor. If the motor cannot follow the step pulses, because they
    come too fast or change their rate too abruptly, it stalls or skips steps, and the position the
    library counts no longer matches the real position.

```cpp
--8<-- "02_SpeedAndAcceleration/02_SpeedAndAcceleration.ino"
```

- `setMaxSpeed()` sets the highest speed the motor reaches, in steps per second.
- `setAcceleration()` sets how fast the speed changes, in steps per second². With 50,000 steps/s² the
  motor reaches 20,000 steps/s after 0.4 s.
- `moveAbs(p)` moves to the position `p`. Positions are counted in steps, from where the motor stood at
  power-up or from the last `setPosition()`.

Which values a motor can take depends on the motor, its load, the driver and the supply voltage. Start
low and raise them until the motor begins to lose steps, then stay well below.

!!! tip "Step rates"
    Microstepping (1) multiplies the step rate. A 200 step motor in 1/16 step mode at 1200 rpm needs
    16 · 200 · 1200 / 60 = 64,000 steps per second.
    { .annotate }

    1.  **Microstepping**

        A driver in microstepping mode divides each full step of the motor (usually 1.8°, 200 per turn)
        into several microsteps, for example 16. The motor runs smoother and the resolution is higher,
        but every microstep needs its own step pulse.

## Where to go from here

| Chapter | Content |
|---|---|
| [Moving a motor](moving-a-motor.md) | Blocking and non-blocking commands, running at a speed, stopping, changes during a move, the state of a motor |
| [Targets with a speed](targets-with-speed.md) | Passing a position at a given speed; chaining moves without a stop |
| [Groups](groups.md) | Several motors on a straight line; rotation with a fixed speed ratio |
| [Motor settings](motor-settings.md) | Pull-in speed, pulse width, polarities, enable pin |
| [Error handling](error-handling.md) | What the commands return, and how to see every warning and error |
| [Examples](examples/index.md) | The example sketches of the library |
