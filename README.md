# TeensyStep
## Problem to be solved 
A lot of interesting projects require the movement of things. The most easy way to implement such a transport system is the use of stepper motors driven by readily available microstep drivers. Those drivers usally expect **step and direction signals** to move the motors. Due to the motor intertia they can not be started at full speed. Instead, steppers need a smooth **acceleration** or deceleration to the final speed. Otherwise the step counters of your application and the real motor positions can run out of sync (steploss errors).
While Microstepping is great to reduce vibration at low motor speeds it requires  **high pulse rates** at even moderate motor speeds. Lets look at a typical example: 
Assume we have a stepper with the usual resolution of 200 steps/rev and want to  run it at a max speed of 1200 rpm. If we want to drive it in 1/16 micro stepping mode we need a pulse rate *r* of: 

&emsp;&emsp;&emsp; *r* = (16 * 200 stp/rev * 1'200 rev/min) / 60 s/min = **64'000 steps/s**

Although there are a few stepper libraries available I found none which is able to handle synchronous and asynchronous accelerated moves of more than one motor at the high pulse rates required for micro stepping motor drivers. 

## Purpose of the Library
**TeensyDelay** is an Arduino library compatible with Teensy 3.0, 3.1, 3.2, 3.5 and 3.6. The library is able to handle synchronous and asynchronous movmenent of   steppers with pulse rates of up to 300'000 steps per second. The following table shows a brief summary of the library spec:

|Description|Specification|
|------|------:|
| Motor speed / pulse rate| 1 - 300'000 stp/s|
| Acceleration|0 - 500'000 stp/s^2|
| Pull in speed (speed up to which no acceleration is done)| 50-10'000 stp/s|
| Synchronous movmenent of motors| up to 10|
| Asynchronous movement of motors| 4 groups of 1 to 10 motors|
| Settable step pulse polarity| Active high or low|
| Settable step pulse width| 1-100µs|
| Settable direction signal polarity| cw / ccw|

## Usage
TBD

## Performance
TBD

