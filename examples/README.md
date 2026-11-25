# Examples

Sketches that show how the library is used, from the smallest one to complete applications. They compile
for all Teensy boards; on boards that have no backend (Teensy 3.x, LC) they do not move a motor.

| Sketch | Concept |
|---|---|
| `01_HelloStepper` | Smallest sketch: commands on the motor, no controller |
| `02_SpeedAndAcceleration` | Maximum speed and acceleration, absolute and relative targets |
| `03_NonBlocking` | Blocking and `Async` commands, status getters, independent motors |
| `04_Rotate` | Run at a speed in steps/s, change it while running, stop |
| `05_ChangeTarget` | New target and new maximum speed during a move |
| `06_TargetWithSpeed` | Target with a speed at the target; chaining moves from the arrival callback |
| `07_Follower` | A motor follows a calculated path (a pendulum), commanded from a timer interrupt |
| `08_Homing` | Emergency stop from a switch interrupt, setting the position |
| `09_HomingPrecise` | A more precise reference: slowly out of the switch, position set where it opens |
| `10_Errors` | Returned `errorCode`, error function, built-in handler (pattern of TeensyTimerTool) |
| `11_MotorSettings` | Pull-in speed, pulse width, step polarity, inverse rotation, DIR setup time, enable pin, separate deceleration |
| `12_Groups` | Synchronized moves; both ways to give the targets; groups created on the fly; speed of a group |
| `13_SyncRotation` | A group rotates with a fixed speed ratio; speed factor for the group |
| `14_RoseFunction` | A slide follows the angle of a spindle and draws a rose curve (v2: RoseFunctionFollower) |
| `15_Winder` | Two independent motors with a ratio that is adjusted while running (v2: Winder) |
| `16_LegacyV2` | An unchanged version 2 sketch on the deprecated wrappers, with a translation table |

## Advanced

`advanced/` holds sketches for the functions of `TeensyStepAdvanced.h`, which are meant for programs that
calculate the motion themselves:

| Sketch | Concept |
|---|---|
| `GroupWaypoints` | A group passes waypoints without stopping (`advanced::GroupPath`): speed at the targets, next move from the arrival function |

## Sketches of version 2

`version2/` holds examples of TeensyStep 2, unchanged: HelloStepper, MultipleSteppers, StepperArray,
Interfacing, Winder and RoseFunctionFollower. They compile against the deprecated wrappers `StepControl` and
`RotateControl` and show that existing sketches keep working. The compiler prints a deprecation hint for the
controllers.

## The classes

The API is documented in the headers: `src/api/Stepper.h`, `src/api/StepperGroup.h`, `src/core/errorCode.h`,
`src/api/GroupPath.h` for the advanced functions, and `src/api/Legacy.h` for the version 2 controllers.
