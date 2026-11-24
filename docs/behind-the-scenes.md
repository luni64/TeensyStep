# Behind the scenes

This chapter explains how the library works inside: how a command becomes step pulses, how the ramp is
calculated, what the hardware timers do, and how a group keeps its motors synchronized. Nothing of it is
needed to use the library. It is written for readers who want to know why the library behaves as it
does, or who want to read its source code.

## The three parts of the library

| Part | Folder | Task |
|---|---|---|
| Interface | `src/api/` | The classes of the sketch: `Stepper`, `StepperGroup`. They check the commands and pass them on |
| Motion core | `src/core/` | Calculates the motion: when the next step is due and which motors make it. Pure calculation, no access to the hardware |
| Backend | `src/backend/` | Makes the pulses on the pins at the calculated times, with the timers of the processor. One backend per processor family |

The motion core knows nothing about timers or pins. It can therefore be compiled and tested on a PC,
where a test checks every single step of a move against the limits of the motor. The backends (1) are
small, and a new processor family needs a new backend only.
{ .annotate }

1.  **Backend**

    The part of a program that works with the hardware, as opposed to the part the user sees. The library
    has one backend for Teensy 4.0, 4.1 and MicroMod and one for Teensy 3.5 and 3.6; the compiler selects
    the one for the board.

## From a command to a step

A `Stepper` object that stands still is only a set of numbers: its pins, its settings and its position.
It occupies no timer and takes no processor time.

A move command changes that:

1. The interface takes a free *channel* and a hardware timer for it. If no timer is free, the
   command returns `noTimer`.
2. The command is handed to the motion core of the channel: "go to position 4000 and stop there".
3. The backend starts the timer. From now on everything happens in the interrupt of the timer.
4. When the motor has arrived, the timer is given back, the position is written back to the `Stepper`
   object, and the channel is free again.

A channel (1) exists for a move, not for a motor: the number of `Stepper` objects in a sketch is not
limited by the timers, only the number of motors that move at the same time.
{ .annotate }

1.  **Channel**

    The library's bookkeeping for one move that is under way: one single motor, or one group. A channel
    holds the motion core of the move and belongs to one hardware timer. There are 16 channels, so 16
    moves can run at the same time.

Between the motion core and the backend there is only one question and one answer. The backend asks
"what is next?", and the core answers with an *event*:

| Part of the event | Meaning |
|---|---|
| a delay | how long to wait after the previous event |
| the motors that step | one bit per motor; a single motor has one bit, a group up to 16 |
| their directions | one bit per motor |
| "target reached" | the arrival function of the sketch is to be called |

The backend waits for the delay, makes the pulses and asks for the next event. A move is a chain of such
events, and the core calculates each one only when it is needed.

## How the ramp is calculated

Many stepper libraries plan a move when it starts: so many steps of acceleration, so many at constant
speed, so many of deceleration. A plan has to be thrown away when something changes on the way.

TeensyStep makes no plan. **For every step, the core decides anew** from three things: where the motor
is, how fast it is, and what the goal is (a position with the speed to have there, or a speed). The
decision for one step is always one of: go faster, keep the speed, go slower.

The rule behind it is the formula for constant acceleration, with the speed v after a distance s, the
speed v₀ before it and the acceleration a:

```
v² = v₀² + 2·a·s
```

For a distance of one step (s = 1) it says: with every step, the *square* of the speed changes by twice
the acceleration. The core therefore keeps the squared speed of the motor, and

- adds 2·a to it for a step of acceleration,
- subtracts 2·d for a step of deceleration (d: the deceleration),
- and from the same formula knows how many steps the motor needs to get from its speed down to the speed
  it shall have at the target. As long as more steps than that are left, it may still go faster or keep
  its speed. When the steps left are just enough, it starts to slow down.

The squared speeds are kept as whole numbers, so that no rounding errors add up over a long ramp. The time of a
step follows from the mean of the speeds at its beginning and its end; this needs one square root per
step on a ramp. At constant speed nothing is calculated at all, and the last step time is used again.

What this method gives:

- **Changes at any time.** A new target, a new maximum speed or a new acceleration is simply a new
  input for the next decision. There is no plan that has to be repaired.
- **Targets with a speed** and the run-up they may need come from the same decision.
- **The pull-in speed** is a special case of it: up to this speed the motor may start, stop and reverse
  without a ramp.

The price is processor time: a decision per step costs more than reading a prepared plan.
[Performance](advanced/performance.md) shows what it amounts to.

## The timers

### One timer for each move

Every channel uses one channel of a hardware timer: of a QuadTimer on Teensy 4.x, of a FlexTimer on
Teensy 3.5 and 3.6. These timers are 16-bit counters that the processor clock drives. The library lets
such a counter run freely and uses its *compare register* (1): the timer requests an interrupt when the
counter reaches the value in that register.
{ .annotate }

1.  **Compare register**

    A register of a timer that holds a number. When the running counter of the timer reaches this number,
    the timer raises an interrupt (or switches a pin). The counter itself keeps running.

Every step has a *due time*, a value of the counter. The interrupt at the due time makes the step, asks
the core for the next event and sets the compare register to the next due time:

```
due time of the next step = due time of this step + delay of the next event
```

The next due time is counted from the previous due time and not from the moment the interrupt was
served. An interrupt that comes a little late, because another interrupt was running, delays one edge
but nothing after it: the errors do not add up, and the mean step rate is exact.

### Which timer is taken

The timers are not reserved in advance. At the start of a move the library looks for a free timer
channel in the hardware itself: one that neither makes PWM nor is in use by another library. It first
takes those that cannot make PWM on any pin, so that `analogWrite()` keeps working where possible. When
the move ends, the registers of the timer channel are put back to the values they had. The rules, the
pins and the cooperation with other timer libraries are described in
[Timers and interrupts](advanced/timers.md).

### Long times with a 16-bit timer

On a Teensy 4.x the counter runs with 150 MHz. A 16-bit counter then runs around every 437 µs: too short
for a motor that makes one step per second.

The library does not slow the counter down for slow motors, since that would make the times of fast
motors coarse. Instead the core hands out **waits**: events with a delay and no step. A step period that
is longer than the timer can count (the library uses at most 400 µs per event on a Teensy 4.x) is cut
into equal pieces. Only the last piece makes the step:

| Speed | Step period | Events per step |
|---|---|---|
| 10,000 steps/s | 100 µs | 1 |
| 1,000 steps/s | 1 ms | 3 (two waits of 333 µs, then the step after another 333 µs) |
| 1 step/s | 1 s | 2,500 |

A wait costs a short interrupt and nothing else. Speeds down to 0.001 steps/s are built this way, with
the full resolution of the timer at every speed.

The waits have a second use. At each of them the core looks whether a new command has arrived. A motor
that creeps at one step per second therefore reacts to `stop()` or to a new speed within a fraction of
a millisecond, and not only at its next step a second later.

### The step pulse

A step pulse has two edges, and both are made by the same timer. No second timer is needed:

| Time | The interrupt |
|---|---|
| due time of the step | sets the STEP pin to its active level, asks the core for the next event, and sets the compare register to the end of the pulse |
| due time + pulse width | sets the STEP pin back, sets the DIR pin for the next step if the direction changes, and sets the compare register to the due time of the next step |

The pins are ordinary digital outputs that the interrupt writes; the timer only provides the times.
This is why any pin can be a STEP or DIR pin, and why a group can step up to 16 pins from one timer.

While the pulse is running, the processor calculates the next event. If the calculation takes longer
than the pulse, the pulse is ended right after it, in the same interrupt. On Teensy 3.5 and 3.6 a pulse
of up to 2 µs is always ended in the interrupt that started it, which saves the second interrupt.

The DIR pin changes at the end of a pulse, so the driver has the whole pause until the next step to
take over the new direction.

## Groups

### One ramp for the whole group

A group does not run a ramp per motor. It treats the move as **one path** from the start positions to
the targets and calculates one ramp for it, exactly as for a single motor. One step of the path is one
step of the member with the longest way, the *lead motor*. The whole group therefore needs one channel
and one timer, whether it has 2 or 16 members.

The limits of the path are derived from the limits of all members. A member that makes half as many
steps as the lead motor moves at half its speed and half its acceleration. The path gets the highest
speed and acceleration at which no member exceeds its own settings.

### Distributing the steps: Bresenham

At every step of the lead motor, the group decides for each other member whether it steps as well. The
method is [Bresenham's line algorithm](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm),
which was invented to draw straight lines on a grid of pixels.

Every member has a counter, the *error*. At each step of the lead motor, the number of steps of the
member is added to it. When the error reaches the number of steps of the lead motor, the member makes a
step and that number is subtracted. Only whole numbers are added and compared, so nothing can drift.

Example: the lead motor makes 5 steps, a second member 2. The error of the member starts at half of 5
(as a whole number: 2), so that it steps in the middle of its intervals:

| Step of the lead motor | 1 | 2 | 3 | 4 | 5 |
|---|:---:|:---:|:---:|:---:|:---:|
| Error after adding 2 | 4 | 6 | 3 | 5 | 2 |
| Reaches 5: the member steps | | yes | | yes | |
| Error after the step | 4 | 1 | 3 | 0 | 2 |
| Position of the member | 0 | 1 | 1 | 2 | 2 |
| Exact position on the line | 0.4 | 0.8 | 1.2 | 1.6 | 2.0 |

The position of the member is the exact position rounded to the nearest whole step. No member is ever
more than half a step away from the line, and this holds wherever the path is and whatever it does: the
ramp decides *when* the next step of the path is made, Bresenham decides *who* steps. That is why a
group stays synchronized while it accelerates, when a speed is changed, and when it is stopped early.

A member always steps at the same moment as the lead motor. A member that is only a little slower than
the lead motor therefore does not step evenly: it steps with the lead motor and leaves a step out from
time to time.

### Rotating groups

A group that rotates with a fixed speed ratio uses the same mechanism: the speeds take the place of the
ways, and the path has no end.

## Why the lists in braces

Groups and their commands are written with lists in braces:

```cpp
StepperGroup xy{x, y};
xy.moveAbs({4000, 1000});
```

In the declarations of the library such a list appears as `std::initializer_list<...>`, which looks
more complicated than what it is. The reasons for this design:

- **Any number of values.** A list in braces can have 2 motors or 16. Without it the library would need
  a separate function for every number of motors, or the sketch would have to fill an array first.
- **No dynamic memory.** The compiler builds the list in place when the function is called, and it is
  gone afterwards. Nothing is allocated with `new` or `malloc`, which matters on a microcontroller that
  runs for weeks. The group copies what it needs into a fixed array of 16 entries.
- **Readable.** The values of the members stand side by side in the order of the members, like the
  coordinates of a point.

A list of motors is declared as `std::initializer_list<std::reference_wrapper<Stepper>>`. The inner
part exists for a technical reason: C++ cannot make a list of references, and a list of `Stepper`
would be a list of *copies* of the motors. `std::reference_wrapper<Stepper>` is a reference that can be
put into a list. The compiler creates it from the name of the motor; a sketch never writes it.

The group itself stores only the addresses of its motors, not the motors. That is why a group is a
light object that can be created for a single command, why a motor can be in several groups, and why
the motors have to exist for as long as the group is used.

For values that are calculated or received at run time, every command also takes an array and its
length, see [Values in braces or in an array](reference/stepper-group.md#values-in-braces-or-in-an-array).

## Commands and the interrupt

The steps are made in an interrupt, while the sketch gives its commands in `loop()` or in an interrupt
of its own. Both work on the same data. To keep them apart, every command and every query disables the
interrupts for the few instructions in which it reads or changes the state of a motor, typically a few
microseconds. A step that becomes due in this time is made right afterwards; since the due times do not
depend on when an interrupt was served, the steps after it are on time again.

This is also the reason for the advice not to call `getPosition()` in a tight loop without need: every
call holds back the step interrupt for a moment.

The **blocking commands** (`moveAbs()`, `stop()`, ...) are the non-blocking ones followed by a loop that
waits until the motor has arrived. While they wait, the steps are made by the interrupt as always, and
the loop calls `yield()`, so that the background tasks of the Teensy core keep running.

The **arrival function** is called from the step interrupt, at the step that reaches the target and
before the next step is calculated. A command given there takes effect for the very next step, which is
why moves can follow each other without a gap. For the same reason an arrival function has to be short:
while it runs, no motor steps.

## Where to find it in the source

| File | Content |
|---|---|
| `src/core/Axis.h`, `Axis.cpp` | the ramp of one motor: the decision per step |
| `src/core/Group.h`, `Group.cpp` | the path of a group and Bresenham |
| `src/backend/Backend.h` | the interface between the library and a backend, with the event |
| `src/backend/teensy4/`, `teensy3/` | the timers and the step interrupt of the boards; the comment at the top of each file describes the timing |
| `src/api/Channels.h`, `Channels.cpp` | the channels: taking and releasing them, enable signals |
| `src/api/Stepper.cpp`, `StepperGroup.cpp` | the commands of the sketch |
