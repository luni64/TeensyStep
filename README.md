# TeensyStep
## Problem to be solved 
A lot of interesting projects require the movement of things. The most easy way to implement such a transport system is the use of stepper motors driven by readily available stepper drivers. Those drivers usally expect simple **step and direction signals** to move the motors. However, due to the motor intertia they can not be started at full speed but need a smooth **acceleration** or deceleration to the final velocity. Otherwise the step counters maintained by the application and the real motor positions can run out of sync (steploss errors). Practically all modern stepper drivers can operate in a so called **microstepping** mode where the mechanically fixed step positions (usually 200 per rev) are subdivided into a number of microsteps. While microstepping is great to reduce vibration at low velocity it requires  **high pulse rates** at even moderate motor speeds. 
Lets look at a typical example: Assume we have a standard 1.8° stepper (200 steps/rev) and  want to  run it at a speed of 1200 rpm in 1/16 micro stepping mode. The required pulse rate *r*  can be calculated by: 

&emsp;&emsp;&emsp; *r* = (16 * 200 stp/rev * 1'200 rev/min) / (60 s/min) = **64'000 steps/s**

Of course there are other Arduino stepper libraries available, e.g. the well known [AccelStepper](http://www.airspayce.com/mikem/arduino/AccelStepper/),&ensp; the standard [Arduino Stepper Library](https://www.arduino.cc/en/Reference/Stepper) or [Laurentiu Badeas Stepper Driver](https://github.com/laurb9/StepperDriver) to name a few. However, I found none which is able to handle synchronous and asynchronous accelerated moves of more than one motor at the high pulse rates required for microstepping drivers. 

## Purpose of the Library
**TeensyDelay** is an efficient Arduino library compatible with Teensy 3.0, 3.1, 3.2, 3.5 and 3.6. The library is able to handle synchronous and asynchronous movement of steppers with pulse rates of up to 300'000 steps per second. The following table shows a summary of the **TeensyDelay** specification:

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
TeensyStep basically uses the following two classes to control the steppers

### Stepper Class
The *Stepper* class encapsulates the physical properties of a stepper motor and its driver:
- Teensy pin numbers the STEP and DIR signals are connected to.
- Maximium speed and pull-in speed of the motor (pull-in speed is the speed up to which the motor can be started without the need for acceleration).
- Step pulse polarity, i.e., if your driver requires active high or active low step pulses.
- Setting for inverted rotation direction

Here a short snippet showing the usage of the stepper class
```c++
...
// define a few motors
Stepper motor_1(2,3); // STEP at pin 2, DIR at pin 3
Stepper motor_2(4,5); // STEP at pin 4, DIR at pin 5
Stepper motor_3(6,7); // STEP at pin 6, DIR at pin 7
...
void setup() {
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
        ...
}
```
The stepper class also keeps track of the current position and accepts the setting of a new target position. A new target position can be set absolutely or relative to the current position. 
```c++
...
uint pos = motor_1.getPosition();   // get current position of the motor
motor_1.setTargetAbs(pos + 100);    // sets the target position 100 steps ahead of the current position
motor_1.setTargetRel(100);          // does the same thing
...
```
### StepControl Class
The *StepControl* class is responsible for actually moving the motors
```c++
...
StepControl<> controller;                     // construct a controller object

controller.move(motor_1);                    // This will move motor_1 to its target position
controller.move(motor_2, motor_3);           // Synchronously moves motor_2 and motor_3 to their target positions

motor_1.setTargetAbs(0);
motor_2.setTargetAbs(0);
motor_3.setTargetAbs(0);
controller.move(motor_1, motor_2, motor_3);  // synchronously move motors back to the starting position
...
```
The *move* commands are blocking, i.e., they will not return until the movement is finished. If you need to do some work while the motors are moving you can use the *moveAsync* commands.

```c++
...
controller.moveAsync(motor_1, motor_2);     // This will start the movement and return immediately
doSomeWork();

while(controller.isRunning()){              // wait until the movement is finished. Do not start a
    delay(10);                              // new movement while another one is still running
}
```

The move amd moveAsync commands always move the motors synchronously. I.e., all motors will always reach their target positions at the same time. If you think of a x/y transport system, the movement will be on a straight line from the current coordinates to the target coordinates. If you need an independent motor movement you can use more than one *StepControl* objects
```c++
...
StepControl<> controller_1;
StepControl<> controller_2;
...
controller_1.moveAsync(motor_2, motor_3);  // starts synchronous movement of motor_2 and motor_3
controller_2.moveAsync(motor_1);           // starts independend movement of motor_1
...
```
## Used Ressources
### StepControl
Each *StepControl* object requires **one PIT Timer** (aka IntervallTimer) and **two channels of a FTM timer**. Since all supported Teensies implement four PIT timers and a FTM0 module which has 8 timer channels the usage is limited to 4 *StepControl* objects existing at the same time. In case you use **TeensyDelay** together with other libraries reserving one ore more IntervallTimer the number of available *StepControl* objects is reduced accordingly. You can use the *isOK()* member to check if the *Controller* object was able to reserve a PIT timer. 



```c++
StepControl<> cnt1;
StepControl<> cnt2;
StepControl<> cnt3;

if(!(cnt1.isOK() && cnt2.isOK() && cnt3.isOK()))
{
    panic();
}
```
A *StepControl* object requires 96 bytes of stack or static memory.
### Stepper
You can define as many *Stepper* objects as you like. *Stepper* objects do not use any system ressources except 56 bytes of stack or static memory.
## Advanced Usage
TBD

## Performance
The table in the figure below shows some information about the performance of the library. To  estimate the generated processor load we need to  know the time it takes for handling acceleration, Bresenham algorithm and switching the STEP and DIR signals. The experiment was done by setting a digital pin to HIGH when the processor enters the corresponding ISR and back to LOW when it leaves it. The actual times were measured with a logic analyzer, the processor load was calculated for various conditions and processors. (The .xlsx spreadsheet can be downloaded  [here](/media/load_calculation.xlsx)). 

![Alt text](/media/load_calculation.PNG?raw=true "Load calculation")

**Summarizing:**
Moving one motor at 100kHz generates a processor load of about 10% for a T3.6 and some 28% for a T3.2. Handling 3 motors at that speed generates a load of 13% and 34% respectively. The results roughly fit to the difference in the clock frequency of both boards. Numbers in the table are linear in motor speed, i.e. half the step rate generates half of the load. 
