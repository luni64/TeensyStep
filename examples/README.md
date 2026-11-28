# Examples

Sketches that show how the library is used, from the smallest one to complete applications. They compile
for all Teensy boards; on boards that have no backend (Teensy 3.0 to 3.2, LC) they do not move a motor.

Every sketch has a `README.md` in its folder: what it shows, what has to be connected, what to expect,
and the page of the [manual](https://luni64.github.io/TeensyStep/) that explains it.

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
| `12_EnableFunction` | An enable signal on a port extender: a function switches the driver instead of a pin |
| `13_Groups` | Synchronized moves; both ways to give the targets; groups created on the fly; speed of a group |
| `14_SyncRotation` | A group rotates with a fixed speed ratio; speed factor for the group |
| `15_RoseFunction` | A slide follows the angle of a spindle and draws a rose curve (v2: RoseFunctionFollower) |
| `16_Winder` | Two independent motors with a ratio that is adjusted while running (v2: Winder) |
| `17_LegacyV2` | An unchanged version 2 sketch on the deprecated wrappers; its README has a translation table |

## Advanced

`advanced/` holds sketches for the functions of `TeensyStepAdvanced.h`, which are meant for programs that
calculate the motion themselves:

| Sketch | Concept |
|---|---|
| `GroupWaypoints` | A group passes waypoints without stopping (`advanced::GroupPath`): moves given ahead, with their speeds in mm/s |
| `PathPlanner` | A small motion planner (`Planner.h`, a part of the example): an x-y table runs a path of points in mm and passes each point as fast as the motors allow |
| `CoreXY` | The same planner on a CoreXY machine (motor A = x + y, motor B = x - y), with speed override and hold from the serial monitor |

## Sketches of version 2

`version2/` holds examples of TeensyStep 2, unchanged: HelloStepper, MultipleSteppers, StepperArray,
Interfacing, Winder and RoseFunctionFollower. They compile against the deprecated wrappers `StepControl` and
`RotateControl` and show that existing sketches keep working. The compiler prints a deprecation hint for the
controllers.

## The classes

The API is documented in the headers: `src/api/Stepper.h`, `src/api/StepperGroup.h`, `src/core/errorCode.h`,
`src/api/GroupPath.h` for the advanced functions, and `src/api/Legacy.h` for the version 2 controllers.
