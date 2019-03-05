---
layout: page
title: TeensyStep
cover: true
---

TeensyStep is a highly efficient Arduino library compatible with the [PJRC](https://www.pjrc.com) boards Teensy 3.0, 3.1, 3.2, 3.5 and 3.6. The library is able to handle synchronized as well as independent movement to a target position. In addition to the target mode TeensyStep allows for continuous rotation of steppers. In both modes pulse rates of up to 300'000 steps per second are possible. 
<!-- {:.message} -->

<!-- One of the design goals of TeensyStep was to provide an intuitive and self explaining programming interface. All movement commands are available in a blocking version and a non blocking (async) version. The blocking commands return from the call only after the movement is finished. The async commands return immediately and the motors finish their  movement in the background. 

Basically TeensyStep works with two types of objects: 
1) A stepper class which encapsulates all physical properties of a stepper motor like maximum speed acceleration, polarity and duration of step pulses etc. In addition to those static properties the stepper objects have methods to set the target position of a movement. 

2) Various controller classes which perform the actual movement. The controller classes are able to move up to 10 steppers synchronously. I.e. the controller adjusts the speed of all motors of one set such that they will reach their targets at the same time.  -->






### Example: Synchronized High Speed movement of two motors

In the video below the two motors are accelerated to 160'000 steps per second. Besides the high step frequency it demonstrates that both motors move synchronized even during acceleration and deceleration phases. 
The shown setup uses a Teensy 3.5, two DRV8825 stepper drivers. Motors are PKP214 from Oriental. 

<iframe width="560" height="315" src="https://www.youtube.com/embed/Fzt75I_Zi14" frameborder="0" allow="accelerometer; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

### Changing motor speed on the fly
This demonstration is done on the same hardware as above. This time a simple potentiometer is used to change the speed of the motors on the fly. Again, both motors move synchronized despite the speed changes. The speed is changed using the acceleration settings of the motors. 

<iframe width="560" height="315" src="https://www.youtube.com/embed/HcaStXmkH2w" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
