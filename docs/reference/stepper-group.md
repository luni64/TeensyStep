# StepperGroup

```cpp
class StepperGroup; // namespace TeensyStep
```

Motors that move synchronized: they start together, arrive together and keep the ratio of their ways
during the whole move. A group is a list of motors; it can be created where it is needed, and a move that was
started goes on when the group object no longer exists. See [Groups](../groups.md).

---

### StepperGroup()

```cpp
StepperGroup();
StepperGroup(std::initializer_list<std::reference_wrapper<Stepper>> motors);
```

Creates an empty group, or a group of the given motors. The second declaration looks more complicated
than it is: it stands for "a list of motors in braces" (1). In a sketch it is written like this:
{ .annotate }

1.  **`std::initializer_list<std::reference_wrapper<Stepper>>`**

    `std::initializer_list` is the type C++ gives to a list of values in braces. `std::reference_wrapper`
    makes it a list of the motors themselves and not of copies. Neither name ever appears in a sketch.
    [Behind the scenes](../behind-the-scenes.md#why-the-lists-in-braces) explains why the library uses
    them.

```cpp
Stepper x(2, 3), y(4, 5), z(6, 7);

StepperGroup xy{x, y};        // a group of two motors
StepperGroup xyz{x, y, z};    // a group of three motors
StepperGroup all;             // an empty group, filled later with add()

StepperGroup{y, z}.moveAbs({0, 0}); // a group that is created for one command
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

## Speed

A group is as fast as its members allow: none of them exceeds its own maximum speed, acceleration,
deceleration or pull-in speed. See [The speed of a group](../groups.md#the-speed-of-a-group).

### overrideSpeed()

```cpp
errorCode overrideSpeed(float factor);
```

The speed factor of the group. What it is a factor of depends on the command the group got last:

- **Moves to targets:** the group runs at `factor` times the speed its members allow, 0 to 1; default 1.
  Allowed at any time, also during a move. The factor belongs to the group object and stays for its
  later moves. 0 holds the group on its line; the move is not over (`isMoving()` stays `true`), a factor
  above 0 lets it go on, and `stop()` ends it. Another factor returns `invalidArgument`.
- **After `rotateAsync()`:** the group runs at `factor` times the speeds given to `rotateAsync()`. A
  negative factor reverses all members. 0 stops the group; a later factor starts it again.

### getSpeed()

```cpp
float getSpeed() const;
```

The current speed of the group in steps/s: that of its fastest member. 0 when the group stands still.

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

### Values in braces or in an array

Every command that takes one value per member exists in two forms. Both do the same; they differ only in
how the values are handed over.

| Form | Declared as | Use it when |
|---|---|---|
| A list in braces | `std::initializer_list<int32_t> targets` | the number of values is written in the sketch |
| An array and its length | `const int32_t* targets, unsigned n` | the values are in an array, for example calculated or received |

```cpp
StepperGroup xyz{x, y, z};

// a list in braces: one value per member, in the order of the members
xyz.moveAbs({4000, 1000, -500});

int32_t a = 4000, b = 1000;
xyz.moveAbs({a, b, a + b});          // variables and expressions are allowed

// an array and its length
int32_t targets[3] = {4000, 1000, -500};
xyz.moveAbs(targets, 3);

int32_t table[10][3];                // ten positions, filled elsewhere
xyz.moveAbs(table[4], 3);            // the fifth of them
```

The number of values has to be the number of members; otherwise the command returns `invalidArgument`
and the group does not move. The values of `rotateAsync()` are speeds and therefore of type `float`:
`xyz.rotateAsync({800, 400, -200})`, or a `float` array and its length.

### moveAbsAsync(), moveAbs()

```cpp
errorCode moveAbsAsync(std::initializer_list<int32_t> targets);
errorCode moveAbsAsync(const int32_t* targets, unsigned n);
errorCode moveAbs(std::initializer_list<int32_t> targets);
errorCode moveAbs(const int32_t* targets, unsigned n);
```

Moves the members synchronized to the positions `targets`, given in the order of the members
[in braces or as an array](#values-in-braces-or-in-an-array). `moveAbsAsync()` returns at once; `moveAbs()`
returns when the group has arrived.

```cpp
xyz.moveAbs({4000, 1000, -500});      // returns when the group has arrived

int32_t targets[] = {4000, 1000, -500};
xyz.moveAbsAsync(targets, 3);         // returns at once
```

### moveRelAsync(), moveRel()

```cpp
errorCode moveRelAsync(std::initializer_list<int32_t> distances);
errorCode moveRelAsync(const int32_t* distances, unsigned n);
errorCode moveRel(std::initializer_list<int32_t> distances);
errorCode moveRel(const int32_t* distances, unsigned n);
```

Like `moveAbs`, with the targets given as distances from the positions at which the members stand:

```cpp
xyz.moveRel({200, 0, -200});          // x 200 steps up, y stays, z 200 steps down
```

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

Runs the members at these speeds (steps/s, the sign is the direction), given in the order of the members
[in braces or as an array](#values-in-braces-or-in-an-array).
The ratio of the speeds is kept exactly. While the group rotates, speeds with the same ratio change the
speed of the group; another ratio returns `notSupported`. All speeds 0: stop.

The speed of a rotating group is changed with [`overrideSpeed()`](#overridespeed).

---

## Stopping

### stopAsync(), stop()

```cpp
errorCode stopAsync();
errorCode stop();
```

Decelerates the group to a standstill; the members stay synchronized. `stop()` returns when the group
stands still.

### emergencyStop()

```cpp
void emergencyStop();
```

Stops the step pulses of all members at once.

---

## State

### isMoving()

```cpp
bool isMoving() const;
```

`true` while the members are moving as this group. `false` if a member moves on its own or in another
group.

## Arrival function

### onArrival()

```cpp
void onArrival(void (*callback)());
void onArrival(void (*callback)(StepperGroup& group));
void onArrival(std::nullptr_t);
```

Sets a function that is called when the group has reached its targets. It is called from an interrupt.
The second form is called with the group object that gave the move, so that one function can serve
several groups; it is not called if that object no longer exists when the move arrives. The last
function set is the one called. `nullptr` removes it.
