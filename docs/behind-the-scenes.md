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
current at every moment, not only at the end. Since the next step is calculated while the pulse of the
current one is made, the position read can be one step ahead of the pulses.

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
640,000 to 10,000, by 5,000 per step: that takes 126 steps. So at every step the calculation compares:

- more than 126 steps left to the target: keep the speed (or accelerate, if the maximum is not reached),
- 126 steps left: start to decelerate.

The motor then arrives at the target at about the pull-in speed and stops there. If the move is too short to
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

### One timer channel for each active move

Every active move has one channel of a hardware timer for itself. The library uses the timer only as a
clock: a counter that runs freely, and one compare register that raises an interrupt when the counter
reaches its value. The pins are not connected to the timer; the interrupt writes them as ordinary
digital outputs. This is why any pin can be a STEP or DIR pin, and why a group can step up to 16 pins
from one timer channel.

The two processor families differ in what a timer channel is:

| | Teensy 4.0, 4.1, MicroMod | Teensy 3.5, 3.6 |
|---|---|---|
| Timers | QuadTimer TMR1 to TMR4, four channels each: 16 channels | FlexTimer FTM0 to FTM3, with 8, 2, 2 and 8 channels: 20 channels |
| Counter | every channel has its own 16-bit counter | all channels of a timer share one counter |
| Set up by the library | the library sets the channel to count the bus clock (150 MHz) without a reload; the counter runs around every 437 µs | nothing: the counter stays as the Teensy core has set it up for PWM (`analogWrite`), counting from 0 to its top value 488 times per second |
| What the library uses | the compare register of the channel and its interrupt | the channel in compare mode without a pin, and its interrupt |
| Longest time for one event | 400 µs | 1 ms |

On Teensy 3.5 and 3.6 the counter is left alone on purpose: the other channels of the same timer keep
making PWM with unchanged frequency. The price is arithmetic modulo the top value of the counter
instead of modulo 2^16.

### Finding a free timer

Timers are not reserved when the sketch starts. At the beginning of every move the library looks for a
timer channel that nobody uses, by reading the hardware:

- a channel whose interrupt is enabled, or that runs in a mode the Teensy core does not set, belongs to
  somebody else and is skipped;
- a channel whose pin is switched to PWM is skipped;
- a timer whose interrupt vector points to the function of another library is skipped as a whole.

Channels that cannot make PWM on any pin are taken first (Teensy 4.x), then channels whose PWM pin is
a pin of the motor itself, then the others. Before the library changes a channel, it stores its
registers; when the move ends, it writes them back. [Timers and interrupts](advanced/timers.md) lists
the channels, their pins and the rules for PWM and other timer libraries.

### Due times

Every event has a *due time*, a value of the counter. The interrupt at the due time executes the event,
gets the next event and sets the compare register to the next due time:

```
due time of the next event = due time of this event + delay of the next event     (modulo the counter range)
```

Three details keep the timing exact:

- **No drift from late interrupts.** The next due time is counted from the previous due time, not from
  the moment the interrupt was served. An interrupt that is served late delays one edge and nothing
  after it.
- **No drift from rounding.** The motion calculation gives the delay in seconds, and the timer counts
  whole counts. The fraction of a count that is cut off is carried over to the next delay, so that the
  mean step rate is exact.
- **Events that are already due.** If the interrupt was served so late that the next due time has
  passed as well, the interrupt function executes that event at once, in the same call, instead of
  waiting for the counter to come around again.

### Long times with a 16-bit timer

A 16-bit counter at 150 MHz runs around every 437 µs: too short for a motor that makes one step per
second. Slowing the counter down for slow motors would make the times of fast motors coarse, and on
Teensy 3.5 and 3.6 the counter is not the library's to change.

The solution is in the motion calculation, not in the timer. The pulse generation tells it the longest
time it can wait in one event (400 µs or 1 ms, see the table above). A step period that is longer is cut
into equal pieces, and the motion calculation hands them out as events **without a step**: *waits*.
Only the last piece makes the step. For a Teensy 4.x:

| Speed | Step period | Events per step |
|---|---|---|
| 10,000 steps/s | 100 µs | 1 |
| 1,000 steps/s | 1 ms | 3: two waits of 333 µs, then the step after another 333 µs |
| 1 step/s | 1 s | 2,500 |

For the pulse generation a wait is an event like any other: it sets the compare register and returns.
Speeds down to 0.001 steps/s are built this way, with the full resolution of the timer at every speed.

The waits have a second use. At each of them the motion calculation notices a command that has arrived
in the meantime and decides the step anew; the part of the period that has already passed counts for the
new step. A motor that creeps at one step per second therefore reacts to `stop()` or to a new speed
within the longest event time, and not only at its next step a second later.

### The step pulse

A step pulse has two edges. Both are made by the same timer channel and the same compare register; no
second timer is needed. A step therefore takes two interrupts:

| Time | The interrupt function |
|---|---|
| due time of the step | sets the STEP pins of the event to their active level; sets the compare register to the end of the pulse; calls the arrival function, if the event reaches a target; gets the next event from the motion calculation |
| due time + pulse width | sets the STEP pins back; sets the DIR pins for the next event; sets the compare register to the due time of the next event |

- **The next event is calculated while the pulse is running.** The time of the pulse is used for the
  calculation instead of being waited for. If the calculation takes longer than the pulse, the pulse is
  ended right after it, in the same call.
- **DIR changes at the end of a pulse**, for the motors that step in the next event. The driver has the
  pause until the next step to take over the new direction.
- **The pause after a pulse** is at least 0.8 µs. Pulse and pause together limit the step rate: about
  170,000 steps/s with the default pulse of 5 µs.
- **Short pulses on Teensy 3.5 and 3.6.** A pulse of up to 2 µs is not worth a second interrupt there:
  the interrupt function waits for its end and makes both edges in one call.

### Several moves at the same time

The channels of one timer share an interrupt. When the compare values of several channels are due
together, the interrupt function first makes the edges of all of them and only then does the rest of
the work, one move after the other. Otherwise the edge of one motor would wait for the calculation of
another.

All step interrupts have the same priority (48: above the default of 128, below the millisecond
interrupt of the Teensy core). They do not interrupt each other. A step of one move that becomes due
while the interrupt of another move is being served is therefore made a little late, by up to the
time one event takes; because of the due times this does not add up.
[Performance](advanced/performance.md) has measured values.

### An emergency stop

`emergencyStop()` does not wait for the next event. It switches the interrupt of the timer channel off
and releases the channel. A pulse that has begun is completed. At that moment the motion calculation is
one event ahead of the pins: the step it has already counted, but that was not made, is taken off the
position again. The position therefore matches the pulses that were made.

## Groups

### One ramp for the whole group

A group does not run a ramp per motor. It treats the move as **one path** from the start positions to
the targets. The length of the path is the number of steps of the member with the longest way, the
*lead motor*, and one step of the path is one step of that member.

The motion along the path is calculated by the same code that calculates a single motor: to the motion
calculation the path is one motor that moves from 0 to the length of the path. Everything said in
[How the ramp is calculated](#how-the-ramp-is-calculated) holds for a group as well. The group needs
one active move and one timer channel, whether it has 2 or 16 members.

### The limits of the path

The path has to be slow enough for every member. A member that makes fewer steps than the lead motor
moves slower in the same proportion, so each of its limits allows the path more:

```
limit of the path from one member = limit of the member × (steps of the path / steps of the member)
```

The path gets the smallest of these values over all members, separately for the maximum speed, the
acceleration, the deceleration and the pull-in speed. A member that does not move sets no limit. If the
group has a speed or an acceleration of its own (`StepperGroup::setMaxSpeed()`, ...), that value is used
instead, as long as it is not above what the members allow.

Example: x makes 4,000 steps and y 1,000, and both motors allow 800 steps/s. The path has 4,000 steps.
x allows the path 800 steps/s, y allows it 800 × 4 = 3,200 steps/s. The path runs at 800 steps/s; x
then runs at 800 and y at 200 steps/s.

### Distributing the steps: Bresenham

At every step of the path, the group decides for each member whether it steps as well. The method is
[Bresenham's line algorithm](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm), which was
invented to draw straight lines on a grid of pixels.

Every member has a counter, the *error*. At each step of the path, the number of steps of the member is
added to it. When the error reaches the length of the path, the member makes a step and the length is
subtracted. Only whole numbers are added and compared, so nothing can drift.

Example: a path of 5 steps, and a member that has to make 2. The error of the member starts at half the
length (as a whole number: 2), so that the member steps in the middle of its intervals:

| Step of the path | 1 | 2 | 3 | 4 | 5 |
|---|:---:|:---:|:---:|:---:|:---:|
| Error after adding 2 | 4 | 6 | 3 | 5 | 2 |
| Reaches 5: the member steps | | yes | | yes | |
| Error after the step | 4 | 1 | 3 | 0 | 2 |
| Position of the member | 0 | 1 | 1 | 2 | 2 |
| Exact position on the line | 0.4 | 0.8 | 1.2 | 1.6 | 2.0 |

The position of the member is the exact position rounded to the nearest whole step: no member is ever
more than half a step away from the line. For the lead motor the same rule gives a step at every step
of the path.

The two tasks are separate: the ramp decides **when** the next step of the path is made, Bresenham
decides **who** steps. The result is one event with a delay and one bit per member. That is why a group
stays synchronized while it accelerates, when its speed is changed, and when it is stopped early: all
of these change only the "when".

A member always steps at the same moment as the lead motor. A member that is only a little slower than
the lead motor therefore does not step evenly: it steps with the lead motor and leaves a step out from
time to time.

### Rotating groups

A group that rotates with a fixed speed ratio uses the same mechanism. The speeds take the place of the
ways: the fastest member is the lead motor, the path is given a fixed length of 2^30, and every member
gets its share of it according to its speed. The motion calculation runs the path at a speed instead of
to a target, so it never ends. The ratios are therefore kept with a resolution of 1 in 2^30, and they
are kept exactly while the group accelerates or reverses, because again only the "when" changes.

## Why the lists in braces

Groups and their commands are written with lists in braces:

```cpp
StepperGroup xy{x, y};
xy.moveAbs({4000, 1000});
```

In the declarations of the library such a list is a `std::initializer_list<...>`. The reasons for this
design:

- **Any number of values.** A list in braces can have 2 entries or 16. Without it the library would
  need a function for every number of motors, or the sketch would have to fill an array first.
- **No dynamic memory.** The compiler builds the list on the stack when the function is called, and it
  is gone afterwards. Nothing is allocated with `new` or `malloc`. The group copies what it needs into
  a fixed array of 16 entries.
- **Readable.** The values of the members stand side by side in the order of the members, like the
  coordinates of a point.

A list of motors is declared as `std::initializer_list<std::reference_wrapper<Stepper>>`. The inner
type exists for a technical reason: C++ cannot make a list of references, and a list of `Stepper` would
be a list of *copies* of the motors. `std::reference_wrapper<Stepper>` is a reference that can be put
into a list. The compiler creates it from the name of the motor; a sketch never writes it.

The group stores only the addresses of its motors. That is why a group is a light object that can be
created for a single command, why a motor can be in several groups, and why the motors have to exist
for as long as the group is used. A move that was started does not need the group object any more:
everything it needs was copied into the active move.

The number of values in a list is checked when the command runs, against the number of members; a
wrong number returns `invalidArgument`. For values that are calculated or received at run time, every
command also takes an array and its length, see
[Values in braces or in an array](reference/stepper-group.md#values-in-braces-or-in-an-array).

## Commands and the interrupt

### Protecting the data

The steps are made in an interrupt, while the sketch gives its commands in `loop()` or in an interrupt
of its own. Both work on the same active move. Every command, and every function that reads the state
of a motor, therefore disables the interrupts while it reads or changes an active move, and restores
the previous interrupt state afterwards. This takes a few microseconds; for a command that starts a
group it is longer, because the limits of the path are calculated in that time.

Because the interrupt state is restored and not simply enabled, the functions can also be called from
interrupts of the sketch.

A step that becomes due while the interrupts are disabled is made right afterwards. This is the reason
for the advice not to call `getPosition()` in a tight loop without need.

### Blocking commands

The blocking commands (`moveAbs()`, `stop()`, ...) are the non-blocking ones followed by a loop that
waits until the motor has arrived. While they wait, the steps are made by the interrupt as always, and
the loop calls `yield()`, so that the background tasks of the Teensy core keep running.

### The arrival function

The arrival function is called from the step interrupt, at the event that reaches the target: after
its STEP edge was made and **before** the next event is calculated. A command that the function gives
changes the goal in the active move, and the next event is already calculated for the new goal. This is
why moves can follow each other without a gap.

For the same reason an arrival function has to be short: while it runs, no step interrupt is served,
and the steps of all motors wait.

### The enable signal

The delays of `setEnablePin()` need no timer of their own either. The on-delay is a series of waits
that the active move hands out before its first step, and the off-delay a series of waits after the
last one. During the off-delay the active move and its timer channel are still occupied, although
`isMoving()` already returns `false`.

## Where to find it in the source

| File | Content |
|---|---|
| `src/core/Axis.h`, `Axis.cpp` | motion calculation of one motor: the decision per step, and the waits for long step periods |
| `src/core/Group.h`, `Group.cpp` | motion calculation of a group: the path, its limits and Bresenham |
| `src/backend/Backend.h` | the interface between the pulse generation (called *backend* in the source) and the rest of the library, with the event |
| `src/backend/teensy4/`, `teensy3/` | the pulse generation of the boards: finding a timer, due times, the step interrupt. The comment at the top of each file describes the timing |
| `src/api/Channels.h`, `Channels.cpp` | the active moves (called *channels* in the source): reserving and releasing them, the enable signals |
| `src/api/Stepper.cpp`, `StepperGroup.cpp` | the commands of the sketch |
