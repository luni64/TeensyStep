# Timers and interrupts

This chapter describes the hardware the library uses on Teensy 4.0, 4.1 and MicroMod, so that it can be
combined with PWM outputs and with other libraries that use timers.

## One timer per move

Every single motor that moves, and every group that moves, needs one hardware timer. The library takes
the timer at the start of the move and gives it back when the move has ended. A motor that stands still
uses no timer, and the number of `Stepper` and `StepperGroup` objects is not limited by the hardware.

If no timer is free, the move command returns the error `noTimer` and the motor is unchanged.

The motors of a group share one timer: the members are stepped together with the motor that has the
longest way. A group of 16 motors therefore needs as little as a single motor.

## Which timers

This and the following sections describe Teensy 4.x; for Teensy 3.5 and 3.6 see
[the last section](#teensy-35-and-36).

The library uses the channels of the four QuadTimers (1) of the processor, TMR1 to TMR4, with four
channels each: 16 timers. It does not use the interval timers (PIT, the timers behind `IntervalTimer`),
the GPT timers or the FlexPWM timers; they stay available to the sketch.
{ .annotate }

1.  **QuadTimer**

    A timer module of the i.MX RT1062 processor with four 16-bit channels. The Teensy core uses some of
    the channels to produce PWM (`analogWrite`) on certain pins.

The step pins are ordinary digital outputs that the timer interrupt switches; the timer only provides the
time. Any pin can therefore be a STEP or DIR pin.

Eight of the 16 channels can make PWM on a pin of the board. So that a motor disturbs PWM as little as
possible, the library takes the timers in this order:

1. channels that cannot make PWM on any pin: TMR4 (all four channels), TMR1 channel 3 and TMR2 channels
   1 to 3;
2. channels whose PWM pin is the STEP or DIR pin of a motor of this move: the motor occupies the pin
   anyway, so nothing else can need that timer;
3. the other channels, as long as their pin is not switched to PWM.

| QuadTimer channel | PWM pin |
|---|---|
| TMR1 channel 0, 1, 2 | 10, 12, 11 |
| TMR2 channel 0 | 13 |
| TMR3 channel 0, 1, 2, 3 | 19, 18, 14, 15 |

## Together with PWM

`analogWrite()` on one of the pins in the table switches the pin to its QuadTimer channel. The library
does not take such a channel: the PWM keeps running, and one timer less is available for motors. The PWM
pins of the FlexPWM timers (all other PWM pins) are not affected at all.

In the opposite order, `analogWrite()` on a pin whose timer channel is being used by a moving motor
changes the registers of that channel and disturbs the motor. Start the PWM outputs before the motors, or
use PWM pins that do not belong to a QuadTimer.

## Together with other timer libraries

Before the library takes a timer channel, it checks in the hardware whether the channel is in use: whether
it runs with a setting that is not the one the core leaves at start, and whether its interrupt is enabled.
A channel in use is skipped. The four channels of a QuadTimer module share one interrupt; if the
interrupt of a module belongs to another library, the library does not use that module at all.

**TeensyTimerTool.** TeensyStep does not depend on TeensyTimerTool, and both can be used in the same
sketch. Timers of TeensyTimerTool that are not QuadTimers (PIT, GPT, TCK) never get in the way. If
TeensyTimerTool uses a QuadTimer module (`TMR1` to `TMR4`), start its timers **before** the first move: the
library then leaves that module alone and uses the other three. In the opposite order TeensyTimerTool
replaces the interrupt of a module that motors are using.

## Interrupts

The step pulses are made in the interrupts of the QuadTimer modules (`IRQ_QTIMER1` to `IRQ_QTIMER4`).
They run at priority 48: above the default priority of 128, so that other interrupts of the sketch do not
delay the steps, and one level below the millisecond interrupt of the core, so that `millis()` keeps
running whatever happens in a step interrupt.

Functions of the sketch that are called from these interrupts, the
[arrival function](../targets-with-speed.md#the-arrival-function) and, in some cases, the
[error function](../error-handling.md#an-error-function), delay the steps of other motors while they run.

An interrupt of the sketch with a priority of 48 or higher (a lower number) delays the step pulses for
as long as it runs. A timer of the sketch that gives commands to the motors should get a lower priority
than the step interrupts.

The library disables the interrupts for a few microseconds while a command changes the state of a motor.

### Changing the priority

A sketch that has interrupts which must not wait for the motors, or that wants its own interrupts to
come first, can set the priority of the step interrupts. The function belongs to the advanced API:

```cpp
#include "TeensyStepAdvanced.h"
using namespace TeensyStep;

void setup()
{
    advanced::setInterruptPriority(128);   // the default priority of the Teensy core
}
```

The numbers are those of the Teensy core (`NVIC_SET_PRIORITY`): 0 is the most urgent priority and 255
the least urgent, in steps of 16. The default is `advanced::defaultInterruptPriority` (48). The new
priority holds for all motors and takes effect at once, also for motors that are moving.

- Interrupts with a lower number than the step interrupts delay the step pulses while they run. At 128
  this includes every interrupt that was left at the default priority of the core; the precision values
  in [Performance](performance.md) were measured with the default of the library.
- With 32 or less, the step interrupts are as urgent as the millisecond interrupt of the core: an
  arrival function that takes long then stops `millis()` for that time.

## Restoring the timers

When a move ends, the library puts all registers of the timer channel back to the values it found, so
that the channel can make PWM again afterwards.

## Teensy 3.5 and 3.6

On these boards the library uses the channels of the four FlexTimers (1), FTM0 to FTM3: 20 channels, of
which up to 16 can be in use at the same time. The interval timers (PIT, the timers behind
`IntervalTimer`) are not used and stay available to the sketch.
{ .annotate }

1.  **FlexTimer**

    A timer of the Kinetis processors with one 16-bit counter and several channels. The Teensy core uses
    the FlexTimers for PWM (`analogWrite()`): every PWM pin belongs to one channel.

All channels of a FlexTimer share one counter, and the core sets this counter up for PWM when the board
starts. The library does not change the counter. It switches a channel that is not needed for PWM to a
compare function without a pin and uses the interrupt of that channel. Therefore:

- **PWM keeps working** on the other pins of the same timer, with unchanged frequency and duty cycle.
- A channel is not taken while its pin makes PWM. A channel that belongs to a pin of the motor itself is
  taken first, since nothing else can need it.
- A timer whose PWM frequency was raised with `analogWriteFrequency()` so far that its counter runs around
  in less than 1.2 ms (above about 830 Hz) is not used for motors. With the default frequency of 488 Hz
  all four timers can be used. Do not change the PWM frequency of a timer while motors use it.
- A timer that counts an encoder (quadrature mode), and a timer whose interrupt another library uses, is
  left alone. So is a channel that another library has set to a mode of its own.
- When a move ends, the channel is set back to what it was.

If no channel is free, the move command returns `noTimer`.

| Timer | Channels | PWM pins |
|---|---|---|
| FTM0 | 8 | 22, 23, 9, 10, 6, 20, 21, 5 |
| FTM1 | 2 | 3, 4 |
| FTM2 | 2 | 29, 30 |
| FTM3 | 8 | 2, 14, 7, 8, 35, 36, 37, 38 |
