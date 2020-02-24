# ESP32Step - Fast Stepper Library for ESP32 boards

Fork of [TeensyStep V2 library](https://luni64.github.io/TeensyStep/) adopted for ESP32 boards. It uses the timers 0, 1, 2 via the esp32-arduino interface.

## Purpose of the Library
**ESP32Step** is an efficient Arduino library compatible with ESP32 and Teensy 3.0, 3.1, 3.2, 3.5 and 3.6. The library is able to handle synchronous and independent movement and continuous rotation of steppers with pulse rates of up to 300'000 steps per second. The following table shows a summary of the **ESP32Step** specifications (taken over from TeensyStep, not yet verified for ESP32):

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

## Architecture
The base idea is to use one reloading interval timer for setting the step pin at a stepper velocity based frequency, one non-reloading (one shot) timer for clearing the step pin and thus defining the pulse width, and one (slower) reloading interval timer for updating the step timer frequency for acceleration / deceleration. 