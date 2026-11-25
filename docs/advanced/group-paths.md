# Paths of a group

A [group](../groups.md) moves on a straight line and stops at its targets. `GroupPath` takes moves of a
group ahead, so that they follow each other without a stop: a path through several points. It is meant
for programs that calculate such a path themselves, for example a motion planner (1), and it leaves more
to the caller than the other commands of the library.
{ .annotate }

1.  **Motion planner**

    A program that turns a list of coordinates and a speed along the path into moves for the motors. It
    looks ahead over the coming moves, decides how fast each corner can be taken, and gives every move
    its speeds. The library does not contain a planner; [Writing a motion planner](motion-planner.md)
    shows how one is built, with an example.

```cpp
#include "TeensyStepAdvanced.h"
using namespace TeensyStep;

StepperGroup xy{x, y};
advanced::GroupPath path(xy);
```

`TeensyStepAdvanced.h` includes `TeensyStep.h`. A `GroupPath` works on a group; arrival function, stop
and state remain those of the [`StepperGroup`](../reference/stepper-group.md).

## A move and its speeds

A move of a path is a straight line to targets, given as positions of the motors in steps, and a
`PathMove` with its speeds:

```cpp
advanced::PathMove move;
move.length        = 100; // the move is 100 mm long
move.speed         = 50;  // 50 mm/s on the way
move.speedAtTarget = 10;  // pass the targets at 10 mm/s
path.add({10'000, 0}, move);
```

`length` says how long the move is in the unit of the sketch, here in mm. All speeds of the move are then
in that unit per second, and its acceleration in that unit per second squared. The library converts them
for the motors (1). It needs nothing but the length for that: no steps per mm, and nothing about how the
motors are arranged.
{ .annotate }

1.  **How the library converts**

    From the targets, the library knows the way of every motor in steps. If the motor with the longest
    way makes 4,000 steps in a move that is 50 mm long, 1 mm of this move is 80 steps of that motor, and
    20 mm/s are 1,600 steps/s. The factor is worked out again for every move.

| Field of `PathMove` | Default | |
|---|---|---|
| `length` | 0 | length of the move in the unit of the sketch. 0: the speeds are in steps of the motor with the longest way, like the speeds of a group |
| `speed` | 0 | highest speed on the way. 0: as fast as the motors allow |
| `speedAtTarget` | 0 | speed at the targets. 0: stop there |
| `acceleration` | 0 | acceleration, also for slowing down. 0: as much as the motors allow |

The maximum speed and the acceleration of the motors stay ceilings for every move. The speed and the
acceleration of the `StepperGroup` (`setMaxSpeed()`, `setAcceleration()`) are not used for the moves of a
path: a move brings its own.

## Moves given ahead

One move runs and one waits. When the group arrives at the targets of a move with a speed, it goes on
with the move that waits: without a gap in time, without a call of the sketch in between, and at the same
speed along the path. The sketch gives the next move while the one before it runs:

```cpp
--8<-- "advanced/GroupWaypoints/GroupWaypoints.ino"
```

`room()` tells how many moves `add()` takes now: 2 while the group stands still, 1 while a move runs and
none waits, 0 while one waits. The sketch above asks in `loop()`. A sketch that cannot be sure to come by
in time sets a function with `onRoom()`; the library calls it, from its interrupt, whenever a move has
arrived and there is room again.

!!! warning "The speeds of the motors jump where two moves meet"
    Within each move the motors stay on a straight line. Where two moves meet, the direction of the line
    changes, and with it the speed of every single motor, without a ramp. The speed at these points has
    to be low enough for the motors to follow (1). The library does not plan ahead across the moves and
    does not round corners.
    { .annotate }

    1.  **How low**

        A motor can change its speed without a ramp by about its pull-in speed, see
        [Motor settings](../motor-settings.md). On a straight continuation, where the direction of the
        line does not change, the speed can be as high as the motors run.

## Rules

- **In time.** The next move has to be given before the move that runs arrives. If none waits when the
  group arrives with a speed, it runs on along its line until a move is given; that move then starts
  where the motors are. A path whose last move has the speed 0 at its targets ends there.
- **No run-up.** A group never leaves its line: if a move is too short to get from the speed at its start
  to the speed at its targets, the group arrives as close to that speed as it can. `add()` then returns
  the warning `targetSpeedNotReached`.
- **The length is taken as given.** A wrong length makes all speeds of the move wrong by the same factor.
- **`stop()` and `emergencyStop()` of the group** end the path. A move that waits is dropped. `stop()`
  brakes along the line of the move that runs, not along the path. To stop on the path, hold it first,
  see [Override and hold](#override-and-hold).
- **A move command of the `StepperGroup`** (`moveAbs()` and the others) is accepted when the group stands
  still or runs on behind its targets; it ends a path with a stop at its targets.
- **The arrival function of the group** is called at the targets of every move of the path.
- **Where `add()` is called.** Preparing a move takes time: about 3 µs on a Teensy 4.1 and 35 µs on a
  Teensy 3.6 for a group of two motors, 5 µs and 80 µs for 16. Called from `loop()`, this holds off no
  step. Called from the function of `onRoom()`, it runs in the interrupt and delays the steps of all
  moving motors by that time, which matters on a Teensy 3.x at high step rates.

## Override and hold

```cpp
path.overrideSpeed(0.5f); // the path at half its speeds
```

Runs the path at a fraction of the speeds of its moves: the move under way, the one that waits and all
that follow, until another factor is set. The factor is at most 1: a path that is planned for its speeds
can always be run slower, but not faster.

```cpp
path.overrideSpeed(0);    // hold: brake and stand on the path
...
path.overrideSpeed(1);    // go on
```

The factor 0 holds the path. The group slows down with the acceleration of its moves and stands on the
path. If the move under way is too short for that, the group goes on into the moves that follow while it
slows down, so the sketch keeps giving moves as before. A factor above 0 lets the path go on from where
it stands.

- **While the path is held,** its moves are kept and further ones are taken. `isMoving()` of the group
  stays `true`, because the targets are not reached; the motors stay enabled. `getSpeed()` returns 0
  when the group stands.
- **To give the path up,** call `stop()` of the group when it stands. The motors are then on the path, at
  the positions `getPosition()` returns, and a new path can start there.
- **The factor stays,** also for the paths that follow: a path that is added while the factor is 0 waits
  for a factor. Move commands of the `StepperGroup` are not held.
- **The speeds of the motors still jump where two moves meet,** but never by more than at the speeds the
  path was planned for.

## Reference

```cpp
struct PathMove;  // namespace TeensyStep::advanced, header TeensyStepAdvanced.h
class GroupPath;
```

### GroupPath()

```cpp
explicit GroupPath(StepperGroup& group);
```

The group has to exist as long as the `GroupPath`. Moves that were started go on when the `GroupPath` is
gone; its function for `onRoom()` is not called any more.

### add()

```cpp
errorCode add(std::initializer_list<int32_t> targets, const PathMove& move);
errorCode add(const int32_t* targets, unsigned n, const PathMove& move);
```

Adds a move to the positions `targets`, in the order of the members of the group, as a list or as an
array with its length `n`. The move starts at once if the group stands still or runs on behind its last
targets, and waits if a move is on its way. Returns at once.

| Returns | |
|---|---|
| `OK` | |
| `speedLimited`, `accelerationLimited` | warning: the motors allow less than the move asks for; their limit is used |
| `targetSpeedNotReached` | warning: the way is too short for the speeds of the move |
| `notSupported` | a move is waiting already (see `room()`), or the group is stopping or rotates |
| `invalidArgument` | wrong number of targets, a negative value, or a move that shall wait and has no way |
| `motorBusy`, `noTimer` | as for the [moves of a group](../reference/stepper-group.md#moves) |

### room()

```cpp
unsigned room() const;
```

The number of moves `add()` takes now: 2, 1 or 0.

### onRoom()

```cpp
void onRoom(void (*function)(GroupPath& path, void* context), void* context = nullptr);
```

Sets a function that is called when a move of the path has arrived at its targets, which is when there
is room for a move again. It gets the `GroupPath` and `context`, a pointer of the sketch's choice, for
example to the object that holds the path. The function is called from an interrupt: keep it short.
`nullptr` removes it.

### overrideSpeed()

```cpp
errorCode overrideSpeed(float factor);
```

See [Override and hold](#override-and-hold). Returns `invalidArgument` for a factor below 0 or above 1.

### getSpeed()

```cpp
float getSpeed() const;
```

The current speed along the path, in the unit of the move under way. 0 at standstill, also when the
path is held.

### movesDone()

```cpp
uint32_t movesDone() const;
```

The number of moves of this path that have arrived at their targets, since the object exists.
