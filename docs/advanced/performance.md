# Performance

The numbers of this chapter were measured on a Teensy 4.1 at 600 MHz with the test suites of the library.
The step signals were measured with a logic analyzer; the step pins were not connected to drivers.

## Processor load

The step pulses are made in interrupts, which take processor time from the sketch. The load was measured
with a counting loop in the sketch that runs once with standing motors and once with moving ones: the
load is the share of loop iterations that is lost.

| Step rate | One motor | Group of three motors |
|---:|:---:|:---:|
| 10,000 steps/s | 4 % | 4 % |
| 50,000 steps/s | 9 % | 11 % |
| 100,000 steps/s | 16 % | 18 % |
| 200,000 steps/s | 29 % | 34 % |
| 300,000 steps/s | 43 % | 50 % |

In the group, the first motor runs at the stated rate, the second at half and the third at a third of it.

The load rises in proportion to the step rate: a step costs about 1.5 µs of processor time. The followers
of a group add little, because they are stepped in the interrupt of the lead motor.

16 motors that run independently at 10,000 to 17,500 steps/s each, 220,000 steps/s in total, load the
processor by about 31 %.

## Precision of the step signal

**One motor, or one group.** From 20 to 300,000 steps/s the mean step rate is exact within 0.1 %, and
every single period is exact within the resolution of the analyzer (0.125 µs). At 1 step/s the periods
are exact within 1 µs. The width of the step pulse is as set.

**Three synchronized motors** at 300,000, 150,000 and 100,000 steps/s produce clean signals; the steps
of the followers coincide with steps of the lead motor.

**Motors that run independently** get in each other's way now and then: a step that is due while the
interrupt of another motor is being served waits for it. The mean step rate stays exact and the delay
does not add up, because a late step is followed by the next one at its proper time.

| Case | Shortest and longest period, against the mean |
|---|:---:|
| 3 independent motors at 50,000 / 47,000 / 53,300 steps/s | −1.3 µs / +1.3 µs |
| 16 independent motors at 10,000 to 17,500 steps/s | about −2.5 µs / +3 µs |

At 50,000 steps/s, 1.3 µs is 6 % of the step period. In a test on a real machine a motor on a spindle ran
smoothly, without lost steps, at up to 45,700 steps/s while six other motors made 221,000 steps/s
together.

## Followers of a group

The followers of a group step only at moments at which the lead motor steps. A follower that makes three
steps for every four of the lead motor therefore makes its steps after one, one and two periods of the
lead motor: its speed is right on average, but its steps are not evenly spaced. The position of the
follower is never more than half a step off the line. The effect is the same as in version 2 and in
other libraries that distribute the steps with Bresenham's line algorithm.
