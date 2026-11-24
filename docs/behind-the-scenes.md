# Behind the scenes

This chapter explains how the library works inside: how a command becomes step pulses, how the ramp is
calculated, what the hardware timers do, and how a group keeps its motors synchronized. Nothing of it is
needed to use the library. It is written for readers who want to know why the library behaves as it
does, or who want to read its source code.

## The three parts of the library

The library consists of three parts with separate tasks:

| Part | Task | Folder of the source |
|---|---|---|
| **Commands** | The classes a sketch uses, `Stepper` and `StepperGroup`. They check a command and start the move | `src/api/` |
| **Motion calculation** | Works out the motion: at which time the next step has to be made, and in which direction | `src/core/` |
| **Pulse generation** | Switches the STEP and DIR pins at the calculated times, with a hardware timer of the processor | `src/backend/` |

```
  sketch                 library                                              driver
            +----------+           +-------------+           +------------+
 moveAbs()  |          |   goal    |   motion    | next step |   pulse    |  STEP, DIR
 ---------> | commands | --------> | calculation | --------> | generation | ----------->
            |          |           |             |           |  (timer)   |
            +----------+           +-------------+           +------------+
```

The motion calculation is arithmetic only: it works with positions, speeds and times and never touches
a timer or a pin. The pulse generation is the only part that depends on the processor. It exists once
for Teensy 4.0, 4.1 and MicroMod and once for Teensy 3.5 and 3.6, and the compiler takes the one that
fits the board. Further processors can be added later without a change to the other two parts.

## From a command to a step

### A motor that stands still

A `Stepper` object that stands still is only a few numbers in memory: its pins, its settings (maximum
speed, acceleration, ...) and its position. It uses no timer and no processor time. A sketch can
therefore have any number of `Stepper` objects.

### The active move

When a motor is told to move, the library needs a place for everything that belongs to this move while
it is under way. This chapter calls that place an **active move**. It holds:

- which motor moves (for a group: which motors),
- the state of the motion calculation: where the motor is now, how fast it is, and where it shall go,
- which hardware timer makes the pulses of this move.

The library has room for 16 active moves. A single motor that moves needs one of them. A group that
moves also needs only one, however many motors it has, because its motors are calculated together. This
is where the limit "16 motors or groups at the same time" comes from.

(In the source code an active move is called a `Channel`. The word is avoided here, because the hardware
timers have channels as well, and the two are different things.)

### What a move command does

Take `motor.moveAbsAsync(4000)` for a motor that stands still:

1. **The command reserves an active move** and looks for a hardware timer that is free. If all 16 active
   moves are taken, or if no timer is free, the command cannot be executed: it returns the error
   `noTimer`, and the motor stays as it was.
2. **The command copies the motor into the active move:** its settings and its current position. Then
   it gives the motion calculation the goal: "go to position 4000 and stop there".
3. **The command starts the timer** and returns. `loop()` runs on.

From this moment the move runs by itself, in the interrupt (1) of the timer. The sketch is not involved
any more, unless it gives a new command.
{ .annotate }

1.  **Interrupt**

    A short function that the processor runs when a piece of hardware asks for it, here a timer that has
    reached a certain time. The processor stops what it is doing, for example `loop()`, runs the
    function, and continues where it was.

### What happens at every step

The timer is set to the time of the next step. When that time has come, the timer calls the pulse
generation, which does three things:

1. It makes the step pulse on the STEP pin.
2. It asks the motion calculation: **"what is next?"** The calculation answers with the next *event*
   (see below): how long to wait, and whether to step then.
3. It sets the timer to that time and returns.

Then the same happens again, step after step. Two points are worth noting:

- **The pulse generation asks; the motion calculation never acts by itself.** It only answers when it is
  asked, and it calculates exactly one step ahead. There is no list of prepared steps.
- **Only the next step is calculated in the interrupt.** There is no planning of the whole move, neither
  here nor anywhere else; how the next step is found is the subject of
  [the next section](#how-the-ramp-is-calculated).

An *event* is the answer of the motion calculation. It contains:

| Part of the event | Meaning |
|---|---|
| a delay | how long to wait after the previous event |
| the motors that step | for a single motor: step or do not step. For a group: which of its motors step |
| their directions | up or down, for each motor that steps |
| "target reached" | set at the step that reaches the target; the arrival function of the sketch is then called |

### Where the position is during a move

While the motor moves, its position is counted in the active move: every event that makes a step also
counts it. `getPosition()` and `getSpeed()` of a moving motor read the values there, so they are
current at every moment, not only at the end.

### The end of a move

When the motor has reached its target and stands still, the motion calculation answers "nothing more".
The pulse generation then stops the timer and puts its registers back as they were, so that the timer
is free again: for the next move, for PWM, or for another library. The final position is copied from
the active move into the `Stepper` object, and the active move is free for the next command.

A command for a motor that is **already moving** needs none of this. It only changes the goal in the
active move, and the next "what is next?" is answered for the new goal.

## How the ramp is calculated

Many stepper libraries plan a move when it starts: so many steps of acceleration, so many at constant
speed, so many of deceleration. A plan has to be thrown away when something changes on the way.

TeensyStep makes no plan. **For every step, the motion calculation decides anew** from three things:
where the motor is, how fast it is, and what the goal is (a position with the speed to have there, or a
speed). The decision for one step is always one of: go faster, keep the speed, go slower.

### The rule for one step

The rule behind the decision is the formula for constant acceleration. It connects the speed v₀ before a
distance s with the speed v after it, for the acceleration a:

```
v² = v₀² + 2·a·s
```

For a distance of one step (s = 1) the formula says: **with every step, the square of the speed changes
by twice the acceleration.** The motion calculation therefore remembers the squared speed of the motor.
To accelerate for one step it adds 2·a to this number; to decelerate it subtracts 2·a.

### An example

A motor with the default settings: maximum speed 800 steps/s, acceleration 2,500 steps/s², pull-in
speed (1) 100 steps/s. It is told to move 4,000 steps.
{ .annotate }

1.  **Pull-in speed**

    The speed up to which the motor can start and stop at once, without a ramp. See
    [Motor settings](motor-settings.md).

The motor starts at the pull-in speed, so the squared speed starts at 100² = 10,000. Every step of
acceleration adds 2 · 2,500 = 5,000:

| After step | Squared speed | Speed (its square root) |
|:---:|:---:|:---:|
| start | 10,000 | 100 steps/s |
| 1 | 15,000 | 122 steps/s |
| 2 | 20,000 | 141 steps/s |
| 3 | 25,000 | 158 steps/s |
| ... | | |
| 126 | 640,000 | 800 steps/s: the maximum speed is reached |

From step 127 on the motor keeps its speed. But when does it have to slow down? The same arithmetic
answers this. To get from 800 steps/s back down to 100 steps/s, the squared speed has to fall from
640,000 to 10,000, by 5,000 per step: that takes 126 steps (the library may differ by a step, because
the last step is treated separately). So at every step the calculation compares:

- more than 126 steps left to the target: keep the speed (or accelerate, if the maximum is not reached),
- 126 steps left: start to decelerate.

The motor then arrives at the target at the pull-in speed and stops there. If the move is too short to
reach the maximum speed, the same comparison makes the motor turn from accelerating to decelerating in
the middle, without a special case.

The time to wait for a step follows from the speed: the step takes one divided by the mean of the speeds
at its beginning and its end. The first step of the example takes 1 / 111 s, about 9 ms; a step at the
maximum speed takes 1 / 800 s = 1.25 ms.

### Why a new command needs nothing special

Suppose the sketch calls `motor.setMaxSpeed(400)` while the motor of the example runs at 800 steps/s.
Nothing is planned again. At the next step the calculation finds that the motor is faster than allowed
and subtracts 5,000 from the squared speed, step after step, until 400 steps/s are reached. The number
of steps needed to stop is smaller at that speed, so the motor will also start to decelerate later.

The same holds for a new target or a new acceleration: they are new inputs of the next decision.

### Details

- The squared speeds are kept as whole numbers, so that no rounding errors add up over a long ramp.
- A step on a ramp needs one square root, to get the speed from its square. At constant speed nothing
  is calculated at all: the time of the last step is used again.

What this method gives:

- **Changes at any time.** A new target, a new maximum speed or a new acceleration is simply a new
  input for the next decision. There is no plan that has to be repaired.
- **Targets with a speed** and the run-up they may need come from the same decision.

The price is processor time: a decision per step costs more than reading a prepared plan.
[Performance](advanced/performance.md) shows what it amounts to.

## The timers

### One timer for each move

Every active move uses one hardware timer. More exactly, it uses one channel of a timer: a QuadTimer of a
Teensy 4.x has four channels and a FlexTimer of a Teensy 3.5 or 3.6 up to eight, and each of them can
time one move. These timers are 16-bit counters that the processor clock drives. The library lets
such a counter run freely and uses its *compare register* (1): the timer requests an interrupt when the
counter reaches the value in that register.
{ .annotate }

1.  **Compare register**

    A register of a timer that holds a number. When the running counter of the timer reaches this number,
    the timer raises an interrupt (or switches a pin). The counter itself keeps running.

Every step has a *due time*, a value of the counter. The interrupt at the due time makes the step, asks
the motion calculation for the next event and sets the compare register to the next due time:

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
motors coarse. Instead the motion calculation hands out **waits**: events with a delay and no step. A step period that
is longer than the timer can count (the library uses at most 400 µs per event on a Teensy 4.x) is cut
into equal pieces. Only the last piece makes the step:

| Speed | Step period | Events per step |
|---|---|---|
| 10,000 steps/s | 100 µs | 1 |
| 1,000 steps/s | 1 ms | 3 (two waits of 333 µs, then the step after another 333 µs) |
| 1 step/s | 1 s | 2,500 |

A wait costs a short interrupt and nothing else. Speeds down to 0.001 steps/s are built this way, with
the full resolution of the timer at every speed.

The waits have a second use. At each of them the motion calculation looks whether a new command has arrived. A motor
that creeps at one step per second therefore reacts to `stop()` or to a new speed within a fraction of
a millisecond, and not only at its next step a second later.

### The step pulse

A step pulse has two edges, and both are made by the same timer. No second timer is needed:

| Time | The interrupt |
|---|---|
| due time of the step | sets the STEP pin to its active level, asks the motion calculation for the next event, and sets the compare register to the end of the pulse |
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
step of the member with the longest way, the *lead motor*. The whole group therefore needs one active move
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
| `src/core/Axis.h`, `Axis.cpp` | motion calculation of one motor: the decision per step |
| `src/core/Group.h`, `Group.cpp` | motion calculation of a group: its path and Bresenham |
| `src/backend/Backend.h` | the interface between the pulse generation (called *backend* in the source) and the rest of the library, with the event |
| `src/backend/teensy4/`, `teensy3/` | the pulse generation of the boards: timers and step interrupt. The comment at the top of each file describes the timing |
| `src/api/Channels.h`, `Channels.cpp` | the active moves (called *channels* in the source): reserving and releasing them, enable signals |
| `src/api/Stepper.cpp`, `StepperGroup.cpp` | the commands of the sketch |
