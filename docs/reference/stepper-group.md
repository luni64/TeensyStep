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
| `invalidArgument` | the number of targets is not the number of members, or the group is empty |
| `motorBusy` | a member is moving on its own or in another group |
| `notSupported` | the group is moving: new targets are accepted when it stands still |
| `noTimer` | no hardware timer is free |

Possible warnings: `speedLimited` and `accelerationLimited`, if the members allow less than the group
asks for.

A group always stops at its targets. For moves that follow each other without a stop see
[Paths of a group](../advanced/group-paths.md).

### moveAbsAsync(), moveAbs()

```cpp
errorCode moveAbsAsync(std::initializer_list<int32_t> targets);
errorCode moveAbsAsync(const int32_t* targets, unsigned n);
errorCode moveAbs(std::initializer_list<int32_t> targets);
errorCode moveAbs(const int32_t* targets, unsigned n);
```

Moves the members on a straight line to the positions `targets`, given in the order of the members as a
list or as an array with its length. `moveAbsAsync()` returns at once; `moveAbs()` returns when the group
has arrived.

```cpp
xyz.moveAbs({4000, 1000, -500});

int32_t targets[] = {4000, 1000, -500};
xyz.moveAbsAsync(targets, 3);
```

### moveRelAsync(), moveRel()

```cpp
errorCode moveRelAsync(std::initializer_list<int32_t> distances);
errorCode moveRelAsync(const int32_t* distances, unsigned n);
errorCode moveRel(std::initializer_list<int32_t> distances);
errorCode moveRel(const int32_t* distances, unsigned n);
```

Like `moveAbs`, with the targets given as distances from the positions at which the members stand.

### moveAsync(), move()

```cpp
errorCode moveAsync();
errorCode move();
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

Sets a function that is called when the group has reached its targets. It is called from an interrupt.
`nullptr` removes it.
