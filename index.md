---
layout: page
title: TeensyStep
cover: true
---
<head> 
  <link rel="stylesheet" href="/TeensyStep/assets/css/myStyle.css">
</head>

TeensyStep is a highly efficient Arduino stepper motor library compatible with the [PJRC](https://www.pjrc.com) boards Teensy 3.0, 3.1, 3.2, 3.5 and 3.6.
The library is able to handle synchronized as well as independent movement of up to **4 groups of up to 10 motors** to their target positions. 
In addition to the **target mode** TeensyStep provides a continuous **rotation mode**. In this mode the steppers can be rotated using fixed rotation speeds. The rotation speed can 
be changed on the fly with keeping the motors in sync. In both modes, pulse rates of up to 300'000 steps per second are possible. 

### Quick Demonstration
In the video below the two motors are accelerated to 160'000 steps per second. Besides the high step frequency it demonstrates that both motors move synchronized not even at their 
target speeds but also during the acceleration and deceleration phases. 

<div class="video-container">
    <iframe width="853" heigth="400" src="https://www.youtube.com/embed/Fzt75I_Zi14" frameborder="0" allow="accelerometer; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>   
</div>

## Why would you use TeensyStep
A lot of interesting projects require the movement of things. An easy way to implement such a transport system is the use of stepper motors driven by readily available stepper drivers. 
Those drivers usually expect simple step and direction signals to move the motors. However, due to the motor inertia they can not be started at full speed but need 
a smooth acceleration or deceleration to the final velocity. Otherwise the motors can skip steps and the step counters maintained by the application  can run out of sync with
the real motor positions (steploss errors). 

Practically all modern stepper drivers can operate in a so called microstepping mode where the mechanically fixed step positions (usually 200 per rev) are electronically subdivided 
into a number of microsteps. While microstepping is great to increase resolution and to reduce vibration at low velocity it requires  high pulse rates at even moderate motor speeds. 

Lets look at a typical example:  
Assume we have a standard 1.8° stepper (200 stp/rev) and  want to  run it at a speed of 1200 rpm in 1/16 micro stepping mode. 
The required pulse rate $$r$$  can be calculated by: 

$$
r = 16\cdot \frac{200\,\text{stp/rev} \cdot 1200\,\text{rev/min}} { 60\,\text{sec/min}} = 64000\,\text{stp/sec}, 
$$

which is pretty demanding. 

Of course, there are other Arduino stepper libraries available.
E.g. the well known [AccelStepper](http://www.airspayce.com/mikem/arduino/AccelStepper/),
the standard [Arduino Stepper Library](https://www.arduino.cc/en/Reference/Stepper) or [Laurentiu Badeas Stepper Driver](https://github.com/laurb9/StepperDriver) to name a few. 
However, I found none which is able to handle accelerated and synchronous moves of more than one motor at the high pulse rates required by microstepping drivers. 



## Key Specifications of TeensyStep

| Description                                | Specification             | Default          |
|:-------------------------------------------|:--------------------------|:---------------- |
| Motor speed / pulse rate 1)                | 1 - 300'000 stp/s         | 800 stp/s        |
| Acceleration  2)                           | 0 - 500'000 stp/s²        | 2500 stp/s²      |
| Pull in speed 3)                           | 50-10'000 stp/s           | 100 stp/s        |
| Synchronous movement                       | up to 10 motors           | -                |
| Independent movement                       | 4 groups of 1 to 10 motors| -                |
| Settable step pulse polarity               | Active HIGH or LOW        | Active HIGH      |
| Settable step pulse width                  | 1-100µs                   | 5µs              |
| Settable direction signal polarity         | cw / ccw                  | cw               |
{:.stretch-table}

[1] Depending on Teensy model  
[2] An acceleration of say 1200 stp/s^2 will accelerate the motor from 0 to 1200 stp/s in 1 second, or from 0 to 2400 stp/s in 2 seconds...  
[3] This is the speed up to which the motor can start without acceleration. 

[Documentation]({{ site.baseurl }}/documentation/)  
[Applications]({{ site.baseurl }}/applications/)  