# StepperGroup

```cpp
class StepperGroup; // namespace TeensyStep
```

Motors that move synchronized: they start together, arrive together and stay on a straight line between
start and target. A group is a list of motors; it can be created where it is needed, and a move that was
started goes on when the group object no longer exists. See [Groups](../groups.md).

---

### StepperGroup()

```cpp
StepperGroup();
StepperGroup(std::initializer_list<std::reference_wrapper<Stepper>> motors);
```

Creates an empty group, or a group of the given motors:

```cpp
StepperGroup xyz{x, y, z};
```

The order of the motors is the order of the targets and speeds in the commands. A group has at most 16
members (`StepperGroup::maxMembers`).

### add(), remove(), clear(), size()

```cpp
errorCode add(Stepper& motor);
void remove(Stepper& motor);
void clear();
unsigned size() const;
```

Change and query the members. `add()` returns `invalidArgument` if the group is full or already has this
motor.

---

## Motion settings

Chainable, and allowed at any time. The values are those of the member with the longest way in a move.
The settings of the members stay upper limits: if a member allows less, the group is slower, and the
warning `speedLimited` or `accelerationLimited` is reported, by the setter during a move and otherwise by
the move command. The values belong to the group object and are used for the moves it starts. See
[Speed and acceleration of a group](../groups.md#speed-and-acceleration-of-a-group).

### setMaxSpeed()

```cpp
StepperGroup& setMaxSpeed(float stepsPerSec);
```

The highest speed of the group. 0 (default): no value of the group; it is as fast as its members allow.

### setAcceleration()

```cpp
StepperGroup& setAcceleration(float stepsPerSec2);
```

How fast the speed of the group changes. 0 (default): as fast as the members allow. The value is also
used for decelerating, until `setDeceleration()` is called.

### setDeceleration()

```cpp
StepperGroup& setDeceleration(float stepsPerSec2);
```

A separate value for decelerating. 0: as fast as the members allow.

---

## Moves

The commands return `errorCode::OK`, a warning or an error; after an error nothing has changed. Possible
errors:

| Code | |
|---|---|
| `invalidArgument` | the number of targets is not the number of members, the group is empty, or a speed is negative |
| `motorBusy` | a member is moving on its own or in another group |
| `notSupported` | the group is on its way to other targets, or it rotates |
| `noTimer` | no hardware timer is free |

Possible warnings: `speedLimited`, `accelerationLimited` (the members allow less than the group asks
for) and `targetSpeedNotReached` (the way is too short for the speed at the targets).

### moveAbsAsync(), moveAbs()

```cpp
errorCode moveAbsAsync(std::initializer_list<int32_t> targets, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveAbsAsync(const int32_t* targets, unsigned n, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveAbs(std::initializer_list<int32_t> targets, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveAbs(const int32_t* targets, unsigned n, float speedAtTarget = 0, float speedAtStart = keepSpeed);
```

Moves the members on a straight line to the positions `targets`. `moveAbsAsync()` returns at once;
`moveAbs()` returns when the group stands still at its targets or, with a speed at the targets, when it
passes them.

| Parameter | |
|---|---|
| `targets` | the positions, in the order of the members, as a list or as an array with its length `n` |
| `speedAtTarget` | 0 (default): stop at the targets. Otherwise: pass the targets at this speed (steps/s of the member with the longest way) and run on along the line. See [Passing targets without a stop](../groups.md#passing-targets-without-a-stop) |
| `speedAtStart` | the speed of the group at the start of the move, for a move that follows another one. Default: the speed the group has |

```cpp
xyz.moveAbs({4000, 1000, -500});

int32_t targets[] = {4000, 1000, -500};
xyz.moveAbsAsync(targets, 3);

xyz.moveAbsAsync({4000, 1000, -500}, 800); // pass the targets at 800 steps/s
```

The command is accepted when the group stands still, and when it has passed targets with a speed and
runs on. While the group is on its way, the same targets with another `speedAtTarget` are accepted.

### moveRelAsync(), moveRel()

```cpp
errorCode moveRelAsync(std::initializer_list<int32_t> distances, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveRelAsync(const int32_t* distances, unsigned n, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveRel(std::initializer_list<int32_t> distances, float speedAtTarget = 0, float speedAtStart = keepSpeed);
errorCode moveRel(const int32_t* distances, unsigned n, float speedAtTarget = 0, float speedAtStart = keepSpeed);
```

Like `moveAbs`, with the targets given as distances from the positions at which the members are.

### moveAsync(), move()

```cpp
errorCode moveAsync(float speedAtTarget = 0);
errorCode move(float speedAtTarget = 0);
```

Moves the members to the targets they remember: those set with `Stepper::setTargetAbs()` or
`setTargetRel()`, or those of the last `moveAbs` / `moveRel` of a group. A member that never got a target
stays where it is.

---

## Rotation

### rotateAsync()

```cpp
errorCode rotateAsync(std::initializer_list<float> speeds);
errorCode rotateAsync(const float* speeds, unsigned n);
```

Runs the members at these speeds (steps/s, the sign is the direction), given in the order of the members.
The ratio of the speeds is kept exactly. While the group rotates, speeds with the same ratio change the
speed of the group; another ratio returns `notSupported`. All speeds 0: stop.

### overrideSpeed()

```cpp
errorCode overrideSpeed(float factor);
```

Runs the group at `factor` times the speeds given to `rotateAsync()`. A negative factor reverses all
members. 0 stops the group; a later factor starts it again. Returns `notSupported` if the group has not
been rotated.

---

## Stopping and state

### stopAsync(), stop()

```cpp
errorCode stopAsync();
errorCode stop();
```

Decelerates the group to a standstill; the members stay on the line. `stop()` returns when the group
stands still.

### emergencyStop()

```cpp
void emergencyStop();
```

Stops the step pulses of all members at once.

### isMoving()

```cpp
bool isMoving() const;
```

`true` while the members are moving as this group. `false` if a member moves on its own or in another
group.

### getSpeed()

```cpp
float getSpeed() const;
```

The current speed of the group in steps/s: that of its fastest member. 0 when the group stands still.

### onArrival()

```cpp
void onArrival(void (*callback)());
```

Sets a function that is called when the group has reached its targets. It is called from an interrupt
and may give the next move of the group. `nullptr` removes it.
