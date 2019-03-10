---
title: Applications
---

## Movement Modes

If you want to control more than one motor there are three different possibilities to do so. Lets assume the standard belt driven x/y transport shown in the figures below. There are two motors (M1 and M2) which move a load (symbolized by the blue ring) on linear guides (pale blue). M1 moves the load in y-direction and M2 in x-direction. 

## Sequential Movement
The simplest thing you can do is to move the motors one after the other:
```c++
...
StepControl<> controller;               

M1.setTargetAbs(targetY);
M2.setTargetAbs(targetX);

controller.move(M1);           // Move M1 to the target position (targetY)
controller.move(M2);           // Move M2_to the target position (targetX) 
...
```
The resulting movement is shown in the figure below.


![Sequential Movement](/media/seqMove.png?raw=true "Sequential Movement")


## Synchronous Movement
If you want to move on a straight line between the two positions StepControl needs to adjust the step rate of one of the motors in relation to the other. Of course, this adjustment is necessary during the complete movement including the acceleration and deceleration phases. StepControl uses [Bresenham's line algorithm](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm) to do this adjustment.

Using synchronous movements with TeensyStep is easy: the controller accepts up to 10 motors in its move and moveAsync command. All motors which are passed to the commands will be moved synchronously.

```c++
...
StepControl<> controller;               

M1.setTargetAbs(targetY);
M2.setTargetAbs(targetX);

controller.move(M1, M2);        // Move M1 _and_ M2 to the target position 
...
```
The resulting movement is shown in the figure below.


![Synchronous Movement](/media/sycMove.png?raw=true "Sequential Movement")


## Independent Movement
Sometimes it is necessary to move one or more motors independently but at the same time as the other motors. Lets extend our example by another transport module (motor M3) which can move left / right only. This transport is supposed to bring fresh samples (red dot) to a transfer area. This transport shall be done independently of the x/y transport which is still  moving around. To accomplish this we need to create a second controller for motor M3.
```c++
StepControl<> controller_1;
StepControl<> controller_2;
...
controller_1.moveAsync(M1, M2);   // synchronous movement of M1 and M2
// ... more controller_1 movements....

M3.setTargetAbs(transferAreaPos);
controller_2.moveAsync(motor_3);  // move M3 to the transfer station, independent of M1/M2 movements which run in parallel
...
while(controller_1.isRunning() || controller_2.isRunning()){  // wait until both controllers finished their movements
    delay(10);
}
// do the sample transfer...
...
```

![Independend Movement](/media/indMove.png?raw=true "Sequential Movement")


## Used Resources
### StepControl
Each *StepControl* object requires **one IntervallTimer** and **two channels of a FTM timer**. Since all supported Teensies implement four PIT timers and the FTM0 module which has 8 timer channels the usage is limited to 4 *StepControl* objects existing at the same time. In case you use **TeensyStep** together with other libraries reserving one ore more IntervallTimer the number of available *StepControl* objects is reduced accordingly. You can use the *isOK()* member to check if the *Controller* object was able to reserve a IntervallTimer. 

```c++
StepControl<> cnt1;
StepControl<> cnt2;
StepControl<> cnt3;

if(!(cnt1.isOk() && cnt2.isOk() && cnt3.isOk()))
{
    panic();
}
```
A *StepControl* object requires 96 bytes of stack or static memory.
### Stepper
You can define as many *Stepper* objects as you like. *Stepper* objects do not use any system resources except 56 bytes of stack or static memory.

## Performance
The table in the figure below shows some information about the performance of the library. To  estimate the generated processor load we need to  know the time it takes for handling acceleration, Bresenham algorithm and switching the STEP and DIR signals. The experiment was done by setting a digital pin to HIGH when the processor enters the corresponding ISR and back to LOW when it leaves it. The actual times were measured with a logic analyzer, the processor load was calculated for various conditions and processors. (The .xlsx spreadsheet can be downloaded  [here](/media/load_calculation.xlsx)). 

![Alt text](/media/load_calculation.PNG?raw=true "Load calculation")

**Summarizing:**
Moving one motor at 100kHz generates a processor load of about 10% for a T3.6 and some 28% for a T3.2. Handling 3 motors at that speed generates a load of 13% and 34% respectively. The results roughly fit to the difference in the clock frequency of both boards. Numbers in the table are linear in motor speed, i.e. half the step rate generates half of the load. 


## Other
* [LICENSE](LICENCE){:.heading.flip-title} 


