# Moving a motor

This chapter covers the commands of a single motor: moving to a position, running at a speed, stopping,
and changing a move while it runs. All commands are member functions of the `Stepper` object.

## Blocking and non-blocking commands

Every move command exists in two forms:

| Blocking | Non-blocking | |
|---|---|---|
| `moveAbs(pos)` | `moveAbsAsync(pos)` | move to the position `pos` |
| `moveRel(delta)` | `moveRelAsync(delta)` | move `delta` steps from the current position |
| `stop()` | `stopAsync()` | decelerate to a standstill |

A blocking command returns when the motor stands still. A command with `Async` in its name returns at
once, and the motor moves in the background (1) while the sketch does other things.
{ .annotate }

1.  **In the background**

    The step pulses are made by a hardware timer and its interrupt. The sketch does not have to call
    anything regularly, and `delay()` or long calculations in `loop()` do not disturb the motor.

In the following sketch two motors move independently of each other while `loop()` blinks the LED and
prints their state:

```cpp
--8<-- "03_NonBlocking/03_NonBlocking.ino"
```

The serial monitor shows a line every 100 ms. Motor `m1` accelerates for half a second and then runs at
its maximum speed, `m2` has the shorter way and arrives first; when both stand still, they are sent back:

```
m1: pos    110  speed   2100   m2: pos    -35  speed   -600
m1: pos    420  speed   4100   m2: pos   -120  speed  -1100
m1: pos    930  speed   6100   m2: pos   -255  speed  -1600
m1: pos   1640  speed   8100   m2: pos   -440  speed  -2100
m1: pos   2550  speed  10000   m2: pos   -675  speed  -2600
m1: pos   3550  speed  10000   m2: pos   -959  speed  -3000
...
m1: pos  29550  speed  10000   m2: pos  -7995  speed   -245
m1: pos  30550  speed  10000   m2: pos  -8000  speed      0
...
m1: pos  49910  speed   1900   m2: pos  -8000  speed      0
m1: pos  50000  speed      0   m2: pos  -8000  speed      0
m1: pos  49890  speed  -2100   m2: pos  -7965  speed    600
```

## The state of a motor

| Function | Returns |
|---|---|
| `getPosition()` | the position in steps |
| `getSpeed()` | the current speed in steps/s; negative while the position counts down, 0 when the motor stands still |
| `isMoving()` | `true` while the motor moves |

The getters can be called at any time, also from interrupts. Do not call them in a tight loop without
need, however: every call takes processor time and holds back the step interrupt for a moment. Read the
values as often as the sketch uses them, as the sketch above does every 100 ms.

`setPositionCounter(pos)` sets the position counter: it gives the place where the motor stands now the number
`pos`, for example 0 after [homing](#homing). The motor does not move. It is not a target: the commands
that send the motor to a position are `moveAbs()` and, in a group, `setTargetAbs()`.

## Running at a speed

`rotateAsync(speed)` runs the motor at a speed in steps/s until another command follows. The sign is the
direction. A further call changes the speed: the motor accelerates or decelerates to the new value, and
if the sign changes, it decelerates, reverses and accelerates again. `rotateAsync(0)` stops the motor,
like `stopAsync()`.

```cpp
--8<-- "04_Rotate/04_Rotate.ino"
```

Speeds are floating point numbers. Values below 1 step/s are possible, down to 0.001 steps/s (one step in
about 17 minutes). The motor does not exceed the speed set with `setMaxSpeed()`; a higher value is limited to it
and reported as a warning, see [Error handling](error-handling.md).

## Changes during a move

A command can be given at any time. If the motor is moving, the new command replaces the current one, and
the motor changes to the new target or speed without stopping first:

- A new target ahead of the motor: the motor runs on and stops there.
- A new target behind the motor, or one that is too close to stop at: the motor decelerates, reverses and
  runs back.
- `setMaxSpeed()`, `setAcceleration()`, `setDeceleration()` and `setPullInSpeed()` take effect at once:
  the motor accelerates or decelerates to the new maximum speed and still stops exactly at its target.

```cpp
--8<-- "05_ChangeTarget/05_ChangeTarget.ino"
```

The commands and getters can also be called from interrupts, for example from a timer that calculates new
targets (example `07_Follower`), or from the encoder of a jog wheel that the motor follows.

## Stopping

| Function | |
|---|---|
| `stopAsync()` | decelerates with the set deceleration and returns at once |
| `stop()` | the same, and returns when the motor stands still |
| `emergencyStop()` | stops the step pulses at once, without deceleration |

After `stop()` the position is valid and a new command can follow immediately.

`emergencyStop()` makes no further step. The position counter matches the pulses that were made, but a
motor that was running fast cannot stand still instantly: it overshoots mechanically and loses steps. After an
emergency stop at speed, the position has to be determined again. `emergencyStopAll()` does the same for
all motors.

## Homing

The library counts steps; it does not know where the mechanics are. A machine usually finds its reference
position with a switch. The following sketch runs the motor slowly towards a switch. The switch triggers
an interrupt, which stops the motor at once; that position is then defined as 0:

```cpp
--8<-- "08_Homing/08_Homing.ino"
```

`emergencyStop()` is used here because it acts immediately and can be called from an interrupt. At the low
homing speed the motor stops without losing steps.

A reference found in this way depends on the speed and on how the switch is hit. Where the reference has
to be precise, the following procedure has proven itself:

1. Run towards the switch and decelerate normally when it is reached. A switch that the mechanics can
   move through, such as a light barrier, is better than one they run into.
2. Run slowly out of the switch again, always in the same direction and at the same speed.
3. Take the place where the switch opens as the reference: its interrupt sets the position counter while
   the motor runs.

The example `09_HomingPrecise` does this.
