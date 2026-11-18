# Paths of a group

A [group](../groups.md) moves on a straight line and stops at its targets. `GroupPath` makes moves of a
group that follow each other without a stop: a path through several points. It is meant for programs that
calculate such a path themselves, for example a motion planner (1), and it leaves more to the caller than
the other commands of the library.
{ .annotate }

1.  **Motion planner**

    A program that turns a list of coordinates and a speed along the path into moves for the motors. It
    looks ahead over the coming moves, decides how fast each corner can be taken, and gives every move
    the speeds at its start and its end. The library does not contain a planner.

```cpp
#include "TeensyStepAdvanced.h"
using namespace TeensyStep;

StepperGroup xy{x, y};
advanced::GroupPath path(xy);
```

`TeensyStepAdvanced.h` includes `TeensyStep.h`. A `GroupPath` works on a group and adds the moves described
here; speed, acceleration, arrival function, stop and state remain those of the
[`StepperGroup`](../reference/stepper-group.md).

## A speed at the targets

```cpp
path.moveAbsAsync({10'000, 0});      // stop at the targets, as a move of the group does
path.moveAbsAsync({10'000, 0}, 500); // pass them at 500 steps/s and run on along the line
```

The speed is that of the lead motor (1), like the maximum speed of a group. The group passes its targets
at this speed and runs on along the line until the next command.
{ .annotate }

1.  **Lead motor**

    The member with the most steps to make in a move. The other members step together with it, each at
    its share of the lead motor's steps.

## The next move

The next move is given when the group has passed its targets, in most cases from the
[arrival function](../groups.md#the-arrival-function-of-a-group) of the group. It starts where the motors
are, so the group goes through the point without a stop:

```cpp
--8<-- "advanced/GroupWaypoints/GroupWaypoints.ino"
```

!!! warning "The speeds of the motors jump at such a point"
    Within each move the motors stay on a straight line. Where two moves meet, the direction of the line
    changes, and with it the speed of every single motor, without a ramp. In the sketch above, y starts
    at 500 steps/s when the group passes the first point. The speed at these points has to be low enough
    for the motors to follow (1). The library does not plan ahead across the moves and does not round
    corners.
    { .annotate }

    1.  **How low**

        A motor can change its speed without a ramp by about its pull-in speed, see
        [Motor settings](../motor-settings.md). On a straight continuation, where the direction of the
        line does not change, the speed can be as high as the motors run.

## Rules

- **New targets** are accepted when the group stands still and when it has passed its targets and runs
  on. In the middle of a move, other targets return the error `notSupported`. This also holds for the
  move commands of the `StepperGroup` itself, which end a path with a stop at their targets.
- **The speed at the targets can be changed during the move** with `setSpeedAtTarget()`: raised when
  further moves follow, or set to 0 to stop at the targets after all.
- **No run-up.** A single motor reverses and takes a run-up if a target is too close for its speed. A
  group never leaves its line: if the way is too short to reach the speed, or to slow down to it, the
  group arrives as close to the speed as it can and changes to it behind the targets. The move command
  then returns the warning `targetSpeedNotReached`. A move that shall stop at its targets, but is too
  short for it, passes the targets, stops and returns to them along the line.
- **After `stop()`** within a move, the same targets continue it.
- **Time in the interrupt.** On a Teensy 4.1 a move command given in the arrival function takes about
  6 µs for a group of two motors and 10 µs for a group of 16.

## The speed at the start of a move

A move that follows another one starts at the speed the group has: the new lead motor starts at the
speed the old one had. A third parameter sets this speed explicitly:

```cpp
path.moveAbsAsync({10'400, 8'000}, 500, 300); // start this move at 300 steps/s, pass its targets at 500
```

A planner needs it because it thinks in the speed along the path, for example in mm/s. The library
measures the speed of a group at its lead motor, and the lead motor can change from one move to the next.
At a corner the same speed along the path is therefore a different number for the move before and the
move after it. The planner converts its speed for each move:

speed for the library = speed along the path × steps of the lead motor / length of the move

## Reference

```cpp
class GroupPath; // namespace TeensyStep::advanced, header TeensyStepAdvanced.h
```

### GroupPath()

```cpp
explicit GroupPath(StepperGroup& group);
```

The group has to exist as long as the `GroupPath`.

### moveAbsAsync(), moveRelAsync()

```cpp
errorCode moveAbsAsync(std::initializer_list<int32_t> targets, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveAbsAsync(const int32_t* targets, unsigned n, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveRelAsync(std::initializer_list<int32_t> distances, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveRelAsync(const int32_t* distances, unsigned n, float speedAtTarget = 0, float speedAtStart = keepSpeed);
```

Starts a move to the positions `targets`, or over the `distances` from where the members are, and returns
at once.

| Parameter | |
|---|---|
| `targets`, `distances` | in the order of the members of the group, as a list or as an array with its length `n` |
| `speedAtTarget` | 0 (default): stop at the targets. Otherwise: pass them at this speed (steps/s of the lead motor) and run on along the line |
| `speedAtStart` | the speed at the start of the move, steps/s of the lead motor. Default (`GroupPath::keepSpeed`): the speed the group has |

Returns the errors of the [moves of a group](../reference/stepper-group.md#moves), and the warnings
`speedLimited`, `accelerationLimited` and `targetSpeedNotReached`. A negative speed returns
`invalidArgument`.

### setSpeedAtTarget()

```cpp
errorCode setSpeedAtTarget(float speedAtTarget);
```

Changes the speed at the targets of the move that is under way. Returns `notSupported` if the group is
not on its way to targets: it stands still, has passed them, or rotates.
