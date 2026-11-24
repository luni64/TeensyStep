# Advanced

The chapters of this section are not needed to use the library. They describe what the library does with
the hardware, how much processor time it takes, where its limits are, and the functions for programs that
calculate the motion themselves.

| Chapter | Content |
|---|---|
| [Timers and interrupts](timers.md) | which timers the library takes, and how it lives together with PWM and other timer libraries |
| [Performance](performance.md) | processor load and precision of the step signals, as measured on a Teensy 4.1 |
| [Limits](limits.md) | range of speeds and positions, number of motors, timing of the signals |
| [Behind the scenes](../behind-the-scenes.md) | how the library works inside: the ramp, the timers, the step pulse, groups |
| [Paths of a group](group-paths.md) | moves of a group that follow each other without a stop, for motion planners (`TeensyStepAdvanced.h`) |
