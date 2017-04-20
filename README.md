# TeensyStep
## Problem to be solved 
A lot of interesting projects require the movement of things. The most easy way to implement such a transport system is the use of stepper motors driven by readily available stepper drivers. Those drivers usally expect **step and direction signals** to move the motors. Due to the motor intertia they can not be started at full speed but need a smooth **acceleration** or deceleration to the final velocity. Otherwise the step counters maintained by the application and the real motor positions can run out of sync (steploss errors). Modern stepper drivers can operate in **microstepping** mode where the mechanically fixed step positions (usually 200 per rev) are subdivided into a number of microsteps. While microstepping is great to reduce vibration at low velocity it requires  **high pulse rates** at even moderate motor speeds. Lets look at a typical example: 
Assume we have a standard 1.8° stepper (200 steps/rev) and  want to  run it at a speed of 1200 rpm in 1/16 micro stepping mode. The required pulse rate *r* for can be calculated by: 

&emsp;&emsp;&emsp; *r* = (16 * 200 stp/rev * 1'200 rev/min) / 60 s/min = **64'000 steps/s**

There are a few Arduino stepper libraries available, e.g. the well known [AccelStepper](http://www.airspayce.com/mikem/arduino/AccelStepper/),&ensp; the standard [Arduino Stepper Library](https://www.arduino.cc/en/Reference/Stepper) or [Laurentiu Badeas Stepper driver](https://github.com/laurb9/StepperDriver) to name a few. However, I found none which is able to handle synchronous and asynchronous accelerated moves of more than one motor at the high pulse rates required for micro stepping drivers. 

## Purpose of the Library
**TeensyDelay** is a highly efficient Arduino library compatible with Teensy 3.0, 3.1, 3.2, 3.5 and 3.6. The library is able to handle synchronous and asynchronous movmenent of steppers with pulse rates of up to 300'000 steps per second. The following table shows a brief summary of the library spec:

| Description                                | Specification             |
|:-------------------------------------------|--------------------------:|
| Motor speed / pulse rate                   | 1 - 300'000 stp/s         |
| Acceleration                               | 0 - 500'000 stp/s^2       |
| Pull in speed (speed up to which no acceleration is done)| 50-10'000 stp/s|
| Synchronous movmenent of motors            | up to 10                  |
| Asynchronous movement of motors            | 4 groups of 1 to 10 motors|
| Settable step pulse polarity               | Active high or low        |
| Settable step pulse width                  | 1-100µs                   |
| Settable direction signal polarity         | cw / ccw                  |




## Usage
TBD

## Performance
TBD

