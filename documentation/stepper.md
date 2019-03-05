---
title:  "Stepper"
description: >  
  Here you should be able to find everything you need to know to use TeensyStep for your projects.
hide_description: true  
---

- seed
{:toc}

## Overview
The stepper class encapsulates all physical properties of a stepper motor 
- Pin numbers of the STEP and DIR signals.
- Maximum and pull-in speed of the motor (pull-in speed is the speed up to which the motor can be started without the need for acceleration).
- Step pulse polarity, i.e., does your driver require active high or active low step pulses.
- Setting for inverted rotation direction

In addition to those static properties the stepper class has methods to set the target position of a movement to an absolute potion or relative to the current position. 


## Constructor
~~~c++
    Stepper(int StepPin, int DirPin);
~~~

Example:
Constructing the Stepper object myStepper with step pulse output on pin 3 and dir signal output on pin 7:
~~~c++
  Stepper myStepper(3,7); 
~~~ 


## Methods

### setTargetAbs

### setTargetRel

### getTarget

### setMaxSpeed
Declaration:<br>
~~~c++
  Stepper& setMaxSpeed(int32_t speed)
~~~

Example:<br>
Set the maximum speed of myStepper to 25000 steps/s:
~~~c++
  myStepper.setMaxSpeed(25000); 
~~~ 

### setAcceleration
Declaration:<br>
~~~c++
  Stepper& setAcceleration(uint32_t)
~~~

Example:<br>
Set the maximum acceleration of myStepper to 15000 steps/s².
~~~c++
  myStepper.setAcceleration(15000); 
~~~ 

An acceleration setting of 15'000 steps/s² would accelerate the stepper to 15'000 steps/s in one second. A speed of 30'000 steps/s would be reached in 2 seconds. 

### setStepPinPolarity
Declaration:<br>
~~~c++
  Stepper& setStepPinPolarity(int p)
~~~

Example:<br>
Set set the step pin polarities of myStepper1 to active high and that of myStepper2 to active low. (Active high setting generates positive pulses, active low setting generates negative pulses.)
~~~c++
  myStepper1.setStepPinPolarity(HIGH);
  myStepper2.setStepPinPolarity(LOW);
~~~ 

### setInverseRotation


### setPosition / getPosition


## Examples
Here a short snippet showing the usage of the stepper class

~~~c++
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
        .setInverseRotation(true)
        .setMaxSpeed(45000)             // stp/s
        .setAcceleration(200000);       // stp/s^2      .
        ...
}
~~~
