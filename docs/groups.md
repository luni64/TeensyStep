# Groups

A `StepperGroup` moves several motors synchronized: they start together, arrive together, and their speeds
keep a fixed ratio during the whole move. Motors that drive an x/y transport, for example, move their
load on a straight line. This chapter first compares the three ways to move more than one motor and then
describes how a group is defined and moved.

## Three ways to move several motors

Assume a belt driven x/y transport: motor M1 moves a load in the y direction, motor M2 in the x direction.

### One after the other

The simplest way is to move the motors in sequence with blocking commands:

```cpp
M1.moveAbs(targetY); // returns when M1 has arrived
M2.moveAbs(targetX);
```

![The load first moves in y, then in x](images/seqMove.png)

### Synchronized

To move on a straight line between the two positions, the step rates of the motors have to keep a fixed
ratio during the whole move, including the acceleration and the deceleration. This is what a group does:

```cpp
StepperGroup xy{M2, M1};
xy.moveAbs({targetX, targetY});
```

![The load moves on a straight line](images/sycMove.png)

### Independent

Motors that have nothing to do with each other are simply given their own commands. They move at the same
time and each with its own ramp. A third motor M3, which brings samples to a transfer position while the
x/y transport is under way, needs nothing special:

```cpp
xy.moveAbsAsync({targetX, targetY}); // M1 and M2, synchronized
M3.moveAbsAsync(transferPosition);   // M3 on its own, at the same time

while (xy.isMoving() || M3.isMoving()) {} // wait for both
```

![A third motor moves independently of the x/y transport](images/indMove.png)

Every single motor and every group that moves needs one hardware timer while it moves. On a Teensy 4.x, 16
motors or groups can move independently at the same time, see
[Timers and interrupts](advanced/timers.md).

## Defining a group

A group is defined from the motors that belong to it, listed in braces:

```cpp
Stepper x(2, 3);
Stepper y(4, 5);
Stepper z(6, 7);

StepperGroup xy{x, y};
StepperGroup xyz{x, y, z};
```

The order of the motors in the braces is the order in which the group takes their targets. A group can
have up to 16 motors. It is only a list: the motors keep their own settings and can still be moved on
their own, and a motor can belong to several groups, see
[Groups are light objects](#groups-are-light-objects).

## Moving a group

```cpp
--8<-- "12_Groups/12_Groups.ino"
```

The targets can be given in two ways:

- **In one call**, in the order of the motors in the group: as positions with `moveAbs({...})` or as
  distances from where the motors stand with `moveRel({...})`.
- **On each motor** with `setTargetAbs()` or `setTargetRel()`, followed by `move()` of the group. A motor
  without a target stays where it is.

The members remember their targets, and the last one given wins: `moveAbs` and `moveRel` of the group
replace targets that were set on the motors before, and a target set on a motor afterwards replaces the
one from the group for the next `move()`. Since the targets are positions, a second `move()` does nothing.

As for a single motor, each command exists in a blocking form (`moveAbs`, `moveRel`, `move`, `stop`) and
a non-blocking form (`moveAbsAsync`, `moveRelAsync`, `moveAsync`, `stopAsync`).

### Synchronized at all times

The members keep the ratio of their ways in every phase of the move: while accelerating, at constant
speed, while decelerating, and when the group is stopped early with `stop()`. No member is ever more than
half a step away from where this ratio puts it.

For the x/y transport of the [pictures above](#synchronized) this means that the load deviates from the
straight line between start and target by at most half a step, and that it stands on the line after an
early stop.

### Speed and acceleration of a group

Without further settings, the group moves as fast as possible without any member exceeding its own
maximum speed or acceleration. The motor with the longest way sets the pace (1); the others follow in
proportion.
{ .annotate }

1.  **Lead motor**

    The member with the most steps to make is the *lead motor* of the move. The other members step
    together with it, each at its share of the lead motor's steps. A member with a short way therefore
    steps less often, but always at the same moment as the lead motor. This is
    [Bresenham's line algorithm](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm).

A group can be given a maximum speed and an acceleration of its own:

```cpp
xy.setMaxSpeed(2'000).setAcceleration(10'000);
xy.moveAbs({4'000, 1'000}); // x runs at 2000 steps/s, y at 500
```

- The values are those of the lead motor, in steps/s and steps/s², as for a single motor.
  `setDeceleration()` sets a separate value for decelerating.
- The settings of the motors stay the upper limits. If a member would have to exceed its own maximum
  speed or acceleration, the group is slower, and the warning `speedLimited` or `accelerationLimited` is
  reported. Set each motor to what it can do, and the group to what the move shall do.
- 0 removes a value: the group is then again as fast as its members allow.
- `getSpeed()` returns the current speed of the group, that of its lead motor.

The speed and the acceleration of the group and of its members can be changed while the group moves; the
group then changes its speed as a whole.

The values belong to the group object. A group that is created for one move takes them in the same
statement: `StepperGroup{y, z}.setMaxSpeed(1'000).moveAbs({0, 0});`

### Groups are light objects

A group is only a list of motors. It can be defined once, like the motors, or created where it is needed:

```cpp
StepperGroup{y, z}.moveAbs({0, 0});
```

A move that was started goes on even when the group object no longer exists.

A motor can be a member of several groups, and it can still be moved on its own. But it can only do one
thing at a time: while it moves in a group, commands for another group with this motor and move commands
for the motor itself return the error `motorBusy`. `stop()` and `emergencyStop()` of a member stop its
whole group.

### The arrival function of a group

`group.onArrival(function)` registers a function that is called when the group has reached its targets.
Like the [arrival function of a motor](targets-with-speed.md#the-arrival-function) it is called from an
interrupt.

A group always stops at its targets. Programs that calculate a path through several points, which the
group shall pass without stopping, use the advanced class `GroupPath`, see
[Paths of a group](advanced/group-paths.md).

## Rotating a group

`rotateAsync()` runs the group members at the given speeds. The ratio of the speeds is kept exactly at all
times, also while the motors accelerate, decelerate or reverse:

```cpp
--8<-- "13_SyncRotation/13_SyncRotation.ino"
```

- A further `rotateAsync()` with speeds of the same ratio changes the speed of the group. Another ratio
  is refused with `notSupported` while the group rotates.
- `overrideSpeed(factor)` runs the group at `factor` times the speeds given to `rotateAsync()`. A negative
  factor reverses all members; 0 stops the group, and a later factor starts it again.

For a ratio that changes while the motors run, use independent motors and adjust the speed of one of
them, as the example `15_Winder` does.
