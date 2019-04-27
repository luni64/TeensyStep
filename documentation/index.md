---
layout: page
title: Class Reference
permalink: /documentation/

classList:
- name: Stepper
  link: stepper/
  description: > 
     The stepper class encapsulates the physical properties of a stepper motor like
     pin numbers of the STEP and DIR signals, speed and acceleration of the motor.

- name: StepControl
  link: stepControl/
  description: > 
    The StepControl class is used to synchronously move up to 10 motors to their target positions.

- name: RotateControl
  link: rotateControl/
  description: > 
   The RotateControl class is used to synchronously rotate up to 10 motors. 
---

The TeensyStep library provides the following classes: 

<table>
{% for class in page.classList %}
  <tr>
    <td> <a href="{{ class.link }}">{{ class.name }}</a></td>
    <td>{{class.description | markdownify}}</td>
  </tr>  
{% endfor %}
</table>

