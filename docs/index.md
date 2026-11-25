# Getting started

TeensyStep moves stepper motors that are connected to drivers with STEP and DIR inputs (1). It produces
the step pulses with the acceleration and deceleration ramps a stepper needs, for single motors and for
groups of motors that move synchronized (2).
{ .annotate }

1.  **STEP and DIR**

    Practically all modern stepper drivers have these two inputs. Every pulse on STEP moves the motor by
    one step (or microstep); the level on DIR sets the direction. See
    [Motors and drivers](motors-and-drivers.md).

2.  **Synchronized**

    The motors of a group start together and arrive together, and their speeds keep a fixed ratio during
    the whole move, also while they accelerate and decelerate. The library distributes the steps with
    [Bresenham's line algorithm](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm): the motor
    with the most steps sets the pace, and each of the others steps whenever it has fallen half a step
    behind its share. See [Groups](groups.md).

The library offers

- commands on the motor itself: `motor.moveAbs(1000)`, `motor.rotateAsync(500)`, `motor.stop()`,
- giving a new target and changing speed or acceleration is possible at any time, also while the motor moves,
- groups of motors that move synchronized: the speeds of the motors keep a fixed ratio at all times,
  also while they accelerate and decelerate. Two motors that drive an x/y transport, for example, move
  their load on a straight line,
- step rates from 0.001 to 300,000 steps per second,
- up to 16 motors or groups that move independently at the same time. A group counts as one, however
  many motors it has (up to 16).

!!! warning "Beta version"
    This manual describes version 3 of TeensyStep, which is a beta (3.0.0-beta.1). **The interface can
    still change in details until version 3.0.0**; the changelog lists every change. It runs on **Teensy 4.0, 4.1 and MicroMod** and on
    **Teensy 3.5 and 3.6**. Teensy 3.0 to 3.2 and LC are not supported: the library compiles there, but
    no motor moves; use version 2.3.4 on these boards. Sketches written for version 2
    still compile, see [Upgrading from version 2](installation.md#upgrading-from-version-2).

Install the library with the Library Manager of the Arduino IDE (*Tools → Manage Libraries…*, search for
TeensyStep, version 3.0.0-beta.1 or later). [Installation and upgrading](installation.md) describes the
installation for PlatformIO and from git, and what to do with sketches written for version 2.

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

Without further settings the motor runs at 800 steps/s and needs about a third of a second to reach this
speed (an acceleration of 2500 steps/s², see the next section). These values are slow, but they work with
any normal stepper.

## Speed and acceleration

A stepper cannot start at full speed: it has to be accelerated, or it stalls (1). The two settings
that matter most are therefore the maximum speed and the acceleration:
{ .annotate }

1.  **Stalling and lost steps**

    The driver has no feedback from the motor. If the motor cannot follow the step pulses, because they
    come too fast or change their rate too abruptly, it usually stalls: it stops and only hums or
    vibrates while the pulses go on. Less often it falls behind and catches on again. It then has lost
    steps, and always a multiple of four full steps, because the magnetic pattern of the motor repeats
    every four full steps: in 1/8 step mode at least 32 steps. In both cases the position the library
    counts no longer matches the real position.

```cpp
--8<-- "02_SpeedAndAcceleration/02_SpeedAndAcceleration.ino"
```

- `setMaxSpeed()` sets the highest speed the motor reaches after the acceleration phase, in steps per
  second.
- `setAcceleration()` sets how fast the speed changes, in steps per second². The sketch sets
  20,000 steps/s²: the motor reaches its maximum speed of 20,000 steps/s after one second.
- The settings can be chained, as in the sketch, and changed at any time.
- `moveAbs(p)` moves to the position `p`. Positions are counted in steps, from where the motor stood at
  power-up or from the last `setPosition()`.

An acceleration of 20,000 steps/s² means that the speed rises by 20,000 steps/s in every second of
accelerating; read the unit as (steps per second) per second. A motor that shall reach 20,000 steps/s
therefore needs one second for it. With twice the acceleration it reaches the same speed in half a
second, with half the acceleration in two seconds. The acceleration changes how soon the motor is at its
speed, not how fast it runs.

The move of the sketch over 40,000 steps takes about three seconds: one to accelerate, one at maximum
speed and one to decelerate.

Which values a motor can take depends on the motor, its load, the driver and the supply voltage. Start
low and raise them until the motor begins to stall or to lose steps, then stay well below.

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
| [Groups](groups.md) | Several motors that move synchronized; rotation with a fixed speed ratio |
| [Motor settings](motor-settings.md) | Pull-in speed, pulse width, polarities, enable pin |
| [Error handling](error-handling.md) | What the commands return, and how to see every warning and error |
| [Examples](examples/index.md) | The example sketches of the library |
