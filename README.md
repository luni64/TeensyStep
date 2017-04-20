# Work in progress, not functional currently!
# .


# TeensyStep
## Problem to be solved 
A lot of interesting projects require the movement of things. The most easy way to implement such a transport system is the use of stepper motors driven by readily available stepper drivers. Those drivers usally expect **step and direction signals** to move the motors. Due to the motor intertia they can not be started at full speed but need a smooth **acceleration** or deceleration to the final velocity. Otherwise the step counters maintained by the application and the real motor positions can run out of sync (steploss errors). Modern stepper drivers can operate in **microstepping** mode where the mechanically fixed step positions (usually 200 per rev) are subdivided into a number of microsteps. While microstepping is great to reduce vibration at low velocity it requires  **high pulse rates** at even moderate motor speeds. Lets look at a typical example: 
Assume we have a standard 1.8° stepper (200 steps/rev) and  want to  run it at a speed of 1200 rpm in 1/16 micro stepping mode. The required pulse rate *r*  can be calculated by: 

&emsp;&emsp;&emsp; *r* = (16 * 200 stp/rev * 1'200 rev/min) / (60 s/min) = **64'000 steps/s**

Of course there are other Arduino stepper libraries available, e.g. the well known [AccelStepper](http://www.airspayce.com/mikem/arduino/AccelStepper/),&ensp; the standard [Arduino Stepper Library](https://www.arduino.cc/en/Reference/Stepper) or [Laurentiu Badeas Stepper Driver](https://github.com/laurb9/StepperDriver) to name a few. However, I found none which is able to handle synchronous and asynchronous accelerated moves of more than one motor at the high pulse rates required for micro stepping drivers. 

## Purpose of the Library
**TeensyDelay** is a highly efficient Arduino library compatible with Teensy 3.0, 3.1, 3.2, 3.5 and 3.6. The library is able to handle synchronous and asynchronous movement of steppers with pulse rates of up to 300'000 steps per second. The following table shows a brief summary of the **TeensyDelay** spec:

| Description                                | Specification             |
|:-------------------------------------------|--------------------------:|
| Motor speed / pulse rate                   | 1 - 300'000 stp/s         |
| Acceleration                               | 0 - 500'000 stp/s^2       |
| Pull in speed (speed up to which no acceleration is required)| 50-10'000 stp/s|
| Synchronous movement of motors             | up to 10                  |
| Asynchronous movement of motors            | 4 groups of 1 to 10 motors|
| Settable step pulse polarity               | Active high or low        |
| Settable step pulse width                  | 1-100µs                   |
| Settable direction signal polarity         | cw / ccw                  |

## Usage
TeensyStep basically uses the following two classes to controll the steppers

**Stepper Class**
The Stepper class encapsulates the physical properties of a stepper motor and its driver:
- Pin numbers where the STEP and DIR signals are connected to.
- Maximium speed and pull-in speed of the motor (pull-in speed is the speed up to which the motor can be started without the need for acceleration).
- Step pulse polarity, i.e., if your driver requires active high or active low step pulses.
- Setting for inverted rotation direction


Here a snippet showing the usage of the stepper class
```c++
...
// define a few motors
Stepper motor_1(2,3); // STEP at pin 2, DIR at pin 3
Stepper motor_2(4,5); // STEP at pin 4, DIR at pin 5
Stepper motor_3(6,7); // STEP at pin 6, DIR at pin 7
...
setup() {
    // Setup some motor properties
    motor_1.setMaxSpeed(64000);         // stp/s
    motor_1.setAcceleration(200000);    // stp/s^2
    
    // you can also use a 'fluent interface' to set the properties
    motor_2
        .setMaxSpeed(30000)             // stp/s
        .setPullInSpeed(800)            // stp/s
        .setAcceleration(150000);       // stp/s^2
    motor_3
        .setStepPulsePolarity(LOW)      //motor connected to an active low driver
        .setInvertedDir(true)
        .setMaxSpeed(45000)             // stp/s
        .setAcceleration(200000);       // stp/s^2      .
}
```
The stepper class also keeps track of the current position and accepts the setting of a new target position. A new target position can be set absolutely or relative to the current position. 

**Controller Class**
TBD, meanwhile see examples

## Performance
The following table shows some information about the performance of the library. To measure the time it takes for calculation of acceleration, Bresenham algorithm and handling the STEP and DIR signals, a digital pin was set to HIGH while the library was in the ISRs. The measured data was used to calculate the generated processor load for various conditions and processors. Summarizing a Teensy 3.6 will needs about 10% processor load for stepping one motor at 100kHz and 13% for 3 motors. A Teensy 3.2 requires 28% and 34% respectively. Numbers are linear in motor speed, i.e. double the speed means double the load. 

![Alt text](/media/load_calculation.PNG?raw=true "Load calculation")

