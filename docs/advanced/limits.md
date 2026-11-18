# Limits

## Speeds and accelerations

| | Range | Default |
|---|---|---|
| Speed | 0.001 to 1,000,000 steps/s | 800 steps/s |
| Acceleration, deceleration | 0.001 to 1,000,000,000 steps/s² | 2500 steps/s² |
| Pull-in speed | 0 to 1,000,000 steps/s | 100 steps/s |

Values outside these ranges are limited to them, and the setter reports the warning `speedLimited` or
`accelerationLimited`.

The ranges are those of the calculation. The step rate a board can produce is lower: on a Teensy 4.1 the
library was tested up to 300,000 steps/s, where the step pulses of one motor take 43 % of the processor
time ([Performance](performance.md)). The step pulse limits the rate as well: the pulse and a pause of
0.8 µs have to fit into the step period.

At low speeds the step periods are as exact as at high ones. A motor at 0.001 steps/s makes one step
every 1000 s.

## Positions

Positions are 32-bit signed numbers: −2,147,483,648 to 2,147,483,647 steps. A motor that rotates
continuously and passes the end of the range goes on at the other end; the step pulses are not affected.
A target on the other side of the wrap is reached the short way.

At 100,000 steps/s the range lasts for about six hours of rotation in one direction. An application that
rotates for longer and needs the position can set it back with `setPosition()` from time to time.

## Number of motors

| | Teensy 4.x |
|---|---|
| `Stepper` and `StepperGroup` objects | not limited by the library |
| Members of a group | 16 |
| Motors or groups moving independently at the same time | 16, less if QuadTimer channels are in use for PWM or by other libraries ([Timers and interrupts](timers.md)) |

## Timing of the signals

| | Range | Default |
|---|---|---|
| Width of the step pulse | 0.8 to 200 µs | 5 µs |
| Time between a change of DIR and the next step | see below | 5 µs |

On Teensy 4.x the DIR signal of a motor changes at the end of the step pulse before the first step in the
new direction. The time to that step is the step period minus the pulse width; at a reversal the motor
runs at its pull-in speed, so this time is long (10 ms at 100 steps/s). The value of `setDirSetupTime()`
is not yet used by the Teensy 4 backend.

## Ramps

The speed changes linearly with time (constant acceleration). The ramp is calculated at every step from
the current position, speed and target; this is why targets and limits can change at any time.

Up to the pull-in speed a motor starts, stops and reverses without a ramp.

## Not in this version

- Groups cannot pass a target at a speed; a group stops at its targets.
- The targets of a group cannot be changed while it moves.
- The timers the library may use cannot be selected by the sketch yet.
- Teensy 3.5 and 3.6: no motor moves yet. Use version 2.3.4.
- Teensy 3.0 to 3.2 and LC: not supported, because these boards have no floating point unit. Use
  version 2.3.4.
