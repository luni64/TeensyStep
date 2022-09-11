# TeensyStep V2.1 - Fast Stepper Library in C

## Purpose of the Library
**TeensyStep** is an efficient Arduino library compatible with Teensy 3.0, 3.1, 3.2, 3.5, 3.6 and STM32F4. The library is able to handle synchronous and independent movement and continuous rotation of steppers with pulse rates of up to 300'000 steps per second. The following table shows a summary of the **TeensyStep** specification:

| Description                                | Specification             | Default          |
|:-------------------------------------------|:-------------------------:|:----------------:|
| Motor speed / pulse rate                   |1 - 300'000 stp/s          |   800 stp/s      |
| Acceleration                               | 0 - 500'000 stp/s^2       |   2500 stp/s^2   |
| Pull in speed                              | 50-10'000 stp/s           | 100 stp/s |
| Synchronous movement of motors             | up to 10                  | -                |
| Independent movement of motors             | 4 groups of 1 to 10 motors| -                |
| Settable step pulse polarity               | Active HIGH or LOW        | Active HIGH      |
| Settable step pulse width                  | 1-100µs                   | 5µs              |
| Settable direction signal polarity         | cw / ccw                  | cw               |

Here a quick demonstration video showing two motors running in sync with 160'000 steps/sec
