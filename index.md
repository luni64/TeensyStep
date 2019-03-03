---
layout: page
title: TeensyStep
cover: true
---

TeensyStep is a highly efficient Arduino library compatible with the [PJRC](https://www.pjrc.com) boards Teensy 3.0, 3.1, 3.2, 3.5 and 3.6. The library is able to handle synchronous and independent movement as well as continuous rotation of steppers with pulse rates of up to 300'000 steps per second. 

One of the design goals of TeensyStep was to provide an intuitive and self explaning programming interface. All movement commands are available in a blocking version and a non blocking (async) version. The blocking commands return from the call only after the movement is finished. The async commands return immediately and the motors finish their  movment in the background. 

Basically TeensyStep works with two types of objects: 
1) A stepper class which encapsulates all physical properties of a stepper motor like maximum speed acceleration, polarity and duration of step pulses etc. In addition to those static properties the stepper objects have methods to set the target position of a movement. 

2) Various controller classes which perform the actual movement. The controller classes are able to move up to 10 steppers synchronously. I.e. the controller adjustst the speed of all motors of one set such that they will reach their targets at the same time. 



