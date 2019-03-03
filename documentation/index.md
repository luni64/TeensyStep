---
# file: index.html
layout: list
title: Documentation
slug:  documentation
#description: >
#  Hyde is a brazen two-column [Jekyll](http://jekyllrb.com) theme.
#  It's based on [Poole](http://getpoole.com), the Jekyll butler.
---

One of the design goals of TeensyStep was to provide an intuitive and self explaning programming interface. All movement commands are available in a blocking version and a non blocking (async) version. The blocking commands return from the call only after the movement is finished. The async commands return immediately and the motors finish their  movment in the background. 

Basically TeensyStep works with two types of objects: 

1) A stepper class which encapsulates all physical properties of a stepper motor like maximum speed acceleration, polarity and duration of step pulses etc. In addition to those static properties the stepper objects have methods to set the target position of a movement. 

2) Various controller classes which perform the actual movement. The controller classes are able to move up to 10 steppers synchronously. I.e. the controller adjustst the speed of all motors of one set such that they will reach their targets at the same time. 