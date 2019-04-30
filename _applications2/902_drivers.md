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
  size:   250

hide_description: true

published: false
---
<p></p>



1. **Voltage Control**   
Apply the rated voltage V to the coils. Usually this voltage is given in the motor datasheet. Alternatively you can calculate it from the coil resistance R and 
the rated coil current `I` by [Ohms law](http://www.ohmslawcalculator.com/ohms-law-calculator)). Motors designed for voltage control require a relatively high resistance 
which typically is achieved by a high number of coil windings leading to a high inductance of the motor (see values in the example datasheet below)
<br><br>
*Advantage:*   
Simple drive electronics. Combined with a unipolar wiring you can drive a voltage controlled stepper by 4 transistors or a 
simple [ULN2003](https://www.st.com/resource/en/datasheet/uln2001.pdf) circuit.   
*Disadvantage:*   
Due to the relatively high inductance of those motors they can not be used for high speed applications
<br><br>
[Here](https://coeleveld.com/arduino-stepper-uln2003a/) you find a nice writeup describing the method.   
![https://coeleveld.com/arduino-stepper-uln2003a/](https://coeleveld.com/wp-content/uploads/2016/10/Arduino_Stepper_unipolar_driver.gif){:height="250"}
![https://coeleveld.com/wp-content/uploads/2016/10/Arduino_Stepper_unipolar_driver.jpg](https://coeleveld.com/wp-content/uploads/2016/10/Arduino_Stepper_unipolar_driver.jpg){:height="250"}



2. **Current Control**   






## TBD

