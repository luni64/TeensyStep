---
title:  Hooking up a Driver
layout: myApp

folder: driver_connection
description: >
  How to connect steppers to a driver, level converting for 3V3 systems, timings.
image:       
  title:  Marjory Collins [Public domain], via Wikimedia Commons
  src:    https://upload.wikimedia.org/wikipedia/commons/3/39/This_man_has_been_a_volunteer_fire_engine_driver_for_twenty-three_years_8d23545v.jpg
  href:   https://commons.wikimedia.org/wiki/File:This_man_has_been_a_volunteer_fire_engine_driver_for_twenty-three_years_8d23545v.jpg
  size:   200

hide_description: true

published: true
---
<p></p>


Basically stepper motors are driven in either constant voltage or constant current mode. Most of the
modern stepper drivers used in hobby applications operate in constant current mode. But there are
exceptions for cost sensitive applications. 

## Constant Current Drivers

Here a few popular examples:

- A4988  
  The IC [A4988](https://www.pololu.com/file/0J450/A4988.pdf) from Allegro is a complete microstepping
   motor driver. It is designed to operate bipolar stepper motors with an output drive capacity of
   up to 35 V and ±2 A. 

- DRV8825  
  The Texas Instruments device [DRV8825](http://www.ti.com/lit/ds/symlink/drv8825.pdf)   The device
  has two H-bridge drivers and a microstepping indexer, and is intended to drive bipolar stepper
  motors. The DRV8825 is capable of driving up to 2.5 A of current from each output   
  
- TB6600HG  
  The [TB6600HG](https://datasheet.octopart.com/TB6600HG-Toshiba-datasheet-103127618.pdf) is a PWM
  chopper-type single-chip bipolar sinusoidal micro-step stepping motor driver capable of driving up
  to 4.5 A at a maximum motor voltage of 50 V and a 



Constant
current drivers are used at a much higher voltage than the rated voltage of the stepper. The reason
for this is 


## Constant Voltage Drivers


Apply the rated voltage V to the coils. Usually this voltage is given in the motor datasheet.
Alternatively you can calculate it from the coil resistance R and the rated coil current `I` by
[Ohms law](http://www.ohmslawcalculator.com/ohms-law-calculator)). Motors designed for voltage
control require a relatively high resistance which typically is achieved by a high number of coil
windings leading to a high inductance of the motor (see values in the example datasheet below)
<br><br>  
*Advantage:*   
Simple drive electronics. Combined with a unipolar wiring you can drive a voltage controlled stepper
by 4 transistors or a simple [ULN2003](https://www.st.com/resource/en/datasheet/uln2001.pdf)
circuit.   
*Disadvantage:*   
Due to the relatively high inductance of those motors they can not be used for high speed
applications <br><br> [Here](https://coeleveld.com/arduino-stepper-uln2003a/) you find a nice
writeup describing the method.   
![https://coeleveld.com/arduino-stepper-uln2003a/](https://coeleveld.com/wp-content/uploads/2016/10/Arduino_Stepper_unipolar_driver.gif){:height="250"}
![https://coeleveld.com/wp-content/uploads/2016/10/Arduino_Stepper_unipolar_driver.jpg](https://coeleveld.com/wp-content/uploads/2016/10/Arduino_Stepper_unipolar_driver.jpg){:height="250"}




## Logic Level Conversion

## Performance

