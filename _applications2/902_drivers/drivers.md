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


Operating a stepper always requires some kind of driving electronics to energize the motor coils. In the
most simple case this can be 4 transistors connected to a unipolar constant voltage stepper.
<!-- 
![https://coeleveld.com/arduino-stepper-uln2003a/](https://coeleveld.com/wp-content/uploads/2016/10/Arduino_Stepper_unipolar_driver.gif){:height="200"}   
<small>([https://coeleveld.com/arduino-stepper-uln2003a/](https://coeleveld.com/arduino-stepper-uln2003a/))<small> -->
Anyway, these days stepper drivers with a much better performance and an easy to use STEP / DIR
interface are readily available for steppers of all types and sizes. 

## Constant Current Drivers

Most of the modern stepper drivers used in hobby applications are developed for bipolar steppers and
operate in constant current mode. In the table below you find a few popular devices and their key specifications. 

[![Driver List](assets/driverlist.png)](assets/driverlist.png)


<!-- <div class="smallTableFont">

| Type     | I<sub>max</sub> [A]<br><small>(heat sink)</small>| U<sub>motor</sub><br>[V]| U<sub>logic</sub><br>[V] | Microsteps | Remarks |
|:---------|:------------------------------|:---------------:|:------------------|:----------:|---------|
|A4988     | 1A (2A)                       | 8.0 - 35        | 3.0 - 5.5V        | up to 1/16 | <small>classic driver for 3d printers</small>   | 
|DRV8825   | 1.5A (2.2A)                   | 8.2 - 45        | 2.5 - 5.25V       | up to 1/32 |  <br><br>    | 
|DRV8824   | 0.75A (1.2A)                  | 8.2 - 45        | 2.5 - 5.25V       | up to 1/32 | <small>low current version of DRV8825</small> |
|TMC2130   | 1.2A                          | 5.5 - 45        | 3.3 - 5V          | up to 1/256| <small>aka 'SilentStepStick'<br> STP/DIR and SPI interface </small> |
|TB6600 |


</div> -->



TBC




## Constant Voltage Drivers

<!-- 
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
 -->



## Logic Level Conversion

## Performance

