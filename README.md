# STM32Step
A simple motion control library, built from luni64's *TeensyStep 2.0* library.\
The main purpose is to keep and improve the original lib functionality, while porting it to STM32 HAL environment. A re-organization of the library is done to have a more intuitive structure (for me at least). Also the use of expensive abstractions (virtual functions) was eliminated and more accurate naming was introduced.

### A possibility
While the library name implies STM32, maybe a platform independent version (*UnifiedStep?*) could emerge if there is demand. The main requirement seems to be a unified timer array API and a unified step generation API. This library provides a starting point for both (taking a lot from TeensyStep), maybe with some work it is achievable.


## Overview
The functionality consists of timing, step generation and speed design (a.k.a. acceleration plan). Timing is outsourced to STM32TimerArray. Step generation has a fully software implementation, `GPIOStepper` represents a stepper motor and creates the waveforms with 'bit banging'. (A `TIMStepper` could also be added representing a stepper motor with hardware aided step generation.)

There are two modes of operation, `SpeedControl` and `PositionControl`. With `SpeedControl` acceleration is planned to transition between speeds. `PositionControl` as the name implies is focused on reaching target positions.
Interactivity is not well supported yet, when a command is given it can't be canceled. For example a position command can't be interrupted halfway, to set a new target instead. Maybe some clever hacks enable that, but it should be built-in functionality.

## Update
Platform conversion is done.\
Basic functionality works, rigorous tests are still needed.



## Versions

- **@ Version 0.3.0**\
First usable version.\
Basicaly a complete rewrite of TeensyStep for STM32 environment.\
Using [STM32TimerArray](https://github.com/RockerM4NHUN/STM32TimerArray) for timing tasks.



## TODO
- [x] Adapt the timer sublibrary to STM32 platform
- [x] Adapt GPIO handling
- [x] Test functionality (only trial runs, no proper testing yet)
- [x] Create a separate library from the timer sublibrary ([STM32TimerArray](https://github.com/RockerM4NHUN/STM32TimerArray))
- [x] Adapt STM32Step to use the new timer library
- [ ] *(Obsolete) Create hardware aided timer library with unchanged API (setup can differ)*
- [ ] Measure performance (load, jitter, maximum performance) with different timer libs (STM32TimerArray, ported TickTimer)
- [ ] Create hardware aided step generator (use timer PWM or pulse generation capability)
- Complement GPIO based step generation, do not overwrite it
- [ ] Add functionality to improve motion control abilities (`MotionControl` class)
- Re-init target during command, without stopping or finishing acceleration
- Motion chain to support position control without the need to stop at every waypoint
- Arc movements? (might be useful for G-code based CNC applications, interpolation also works)
