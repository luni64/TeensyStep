# Examples

The library comes with example sketches, from the smallest one to complete applications. In the Arduino
IDE they are found under *File → Examples → TeensyStep*. All of them use pins 2 and 3 for STEP and DIR of
the first motor; change the pin numbers to match the wiring. Every sketch has a `README.md` in its
folder that says what it shows, what has to be connected and what to expect.

## Basics

| Sketch | Shows |
|---|---|
| `01_HelloStepper` | the smallest sketch: commands on the motor |
| `02_SpeedAndAcceleration` | maximum speed and acceleration, absolute and relative targets |
| `03_NonBlocking` | blocking and `Async` commands, the state of a motor, independent motors |
| `04_Rotate` | running at a speed, changing it while running, stopping |
| `05_ChangeTarget` | a new target and a new maximum speed during a move |
| `06_TargetWithSpeed` | a target with a speed; chaining moves from the arrival function |
| `07_Follower` | a motor follows a calculated path (a pendulum), commanded from a timer interrupt |
| `08_Homing` | emergency stop from a switch interrupt, setting the position |
| `09_HomingPrecise` | a more precise reference: slowly out of the switch, position set where it opens |
| `10_Errors` | returned codes, an error function, the ready-made handler |
| `11_MotorSettings` | pull-in speed, pulse width, polarities, DIR setup time, enable pin, separate deceleration |
| `12_EnableFunction` | an enable signal on a port extender: a function switches the driver instead of a pin |

## Groups

| Sketch | Shows |
|---|---|
| `13_Groups` | synchronized moves; the ways to give the targets; groups created where they are needed; the speed of a group |
| `14_SyncRotation` | a group rotating with a fixed speed ratio; the speed factor of a group |

## Applications

| Sketch | Shows |
|---|---|
| `15_RoseFunction` | a slide follows the angle of a spindle and draws a rose curve |
| `16_Winder` | a coil winder: two independent motors with a ratio that is trimmed while they run |

### Rose curve (`15_RoseFunction`)

A spindle rotates at constant speed. A slide moves in and out depending on the angle of the spindle:
r = amplitude · cos(k · angle). A pen on the slide then draws a rose curve on a disk that turns with the
spindle.

```cpp
--8<-- "15_RoseFunction/15_RoseFunction.ino"
```

The slide [follows a calculated path](../targets-with-speed.md#following-a-calculated-path). Every 25 ms
a timer reads the angle of the spindle, calculates where the slide has to be 25 ms later and how fast it
moves there, and gives both to the slide as a target with a speed. The slide therefore does not stop at
each point but runs through the points smoothly. The speed is given without a sign, so that the slide
passes each point in the direction in which it comes to it.

The commands of the library may be called from a timer interrupt, as it is done here. Such a timer gets
a low priority, so that it does not delay the step pulses, see
[Interrupts](../advanced/timers.md#interrupts).

### Coil winder (`16_Winder`)

A string or coil winder has two motors: the spindle rotates the core, and the feeder moves the wire
along it. The pitch (1) is the ratio of the two speeds. It has to stay constant while the spindle speeds
up or slows down, and it shall be trimmable while the machine winds.
{ .annotate }

1.  **Pitch**

    The way of the feeder for each turn of the spindle, in mm. It sets how close the turns of the wire
    lie to each other.

```cpp
--8<-- "16_Winder/16_Winder.ino"
```

A [rotating group](../groups.md#rotating-a-group) keeps a ratio exactly, but the ratio cannot change
while the group runs. The two motors of the winder therefore run independently, and the sketch keeps the
ratio itself:

- **The spindle speed changes** (`setSpindleSpeed()`): both motors get their new speed at the same
  moment, and the acceleration of the feeder is set to that of the spindle times the ratio. Both ramps
  then take the same time, and the pitch stays constant during the ramp.
- **The pitch changes** (`setPitch()`): only the feeder gets a new speed, with an acceleration of its
  own.

The sketch winds at 1125 rpm, trims the pitch three times, slows down to 300 rpm and stops. It prints
the time, both speeds and the pitch they make, for a diagram.

## Advanced

| Sketch | Shows |
|---|---|
| `advanced/GroupWaypoints` | a group that passes waypoints without stopping, with `advanced::GroupPath`; see [Paths of a group](../advanced/group-paths.md) |
| `advanced/PathPlanner` | a small motion planner: an x-y table runs a path of points in mm and passes each point as fast as the motors allow; see [Writing a motion planner](../advanced/motion-planner.md) |
| `advanced/CoreXY` | the same planner on a CoreXY machine, with speed override and hold from the serial monitor |

## Sketches of version 2

| Sketch | Shows |
|---|---|
| `17_LegacyV2` | an unchanged version 2 sketch on the deprecated controllers; the translation table is in [Upgrading](../installation.md) |
| `version2/` | examples of TeensyStep 2, unchanged: HelloStepper, MultipleSteppers, StepperArray, Interfacing, Winder, RoseFunctionFollower |

See [Upgrading from version 2](../installation.md#upgrading-from-version-2).

## The simplest example

```cpp
--8<-- "01_HelloStepper/01_HelloStepper.ino"
```
