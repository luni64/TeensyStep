# Examples

The library comes with example sketches, from the smallest one to complete applications. In the Arduino
IDE they are found under *File → Examples → TeensyStep*. All of them use pins 2 and 3 for STEP and DIR of
the first motor; change the pin numbers to match the wiring.

## Basics

| Sketch | Shows |
|---|---|
| `01_HelloStepper` | the smallest sketch: commands on the motor |
| `02_SpeedAndAcceleration` | maximum speed and acceleration, absolute and relative targets |
| `03_NonBlocking` | blocking and `Async` commands, the state of a motor, independent motors |
| `04_Rotate` | running at a speed, changing it while running, stopping |
| `05_ChangeTarget` | a new target and a new maximum speed during a move |
| `06_TargetWithSpeed` | a target with a speed; chaining moves from the arrival function |
| `09_Homing` | emergency stop from a switch interrupt, setting the position |
| `10_Errors` | returned codes, an error function, the ready-made handler |
| `12_MotorSettings` | pull-in speed, pulse width, polarities, DIR setup time, enable pin, separate deceleration |

## Groups

| Sketch | Shows |
|---|---|
| `07_Groups` | synchronized moves; the ways to give the targets; groups created where they are needed; the speed of a group |
| `13_SyncRotation` | a group rotating with a fixed speed ratio; the speed factor of a group |
| `08_GroupWaypoints` | a group that passes waypoints without stopping: a speed at the targets, the next move from the arrival function |

## Applications

| Sketch | Shows |
|---|---|
| `14_Follower` | a motor follows a calculated path, commanded from a timer interrupt |
| `15_Winder` | a coil winder: two independent motors with a ratio that is trimmed while they run |

## Sketches of version 2

| Sketch | Shows |
|---|---|
| `11_LegacyV2` | an unchanged version 2 sketch on the deprecated controllers, with a translation table |
| `version2/` | examples of TeensyStep 2, unchanged: HelloStepper, MultipleSteppers, StepperArray, Interfacing, Winder, RoseFunctionFollower |

See [Upgrading from version 2](../upgrading.md).

## The simplest example

```cpp
--8<-- "01_HelloStepper/01_HelloStepper.ino"
```
