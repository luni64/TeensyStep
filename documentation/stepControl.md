---
layout:      mypage

className:   StepControl
includeFile: TeensyStep.h

chapters:
- name: Construction
  anchor: constructor
  methods: 

  - name: StepControl
    shortDesc: Constructs a StepControl object with selectable values for the step pulse width (µs) and the speed update period (µs). During acceleration and deceleration, the controller updates the motor speed periodically. The period of this updates is set by the speedUpdatePeriod parameter (500µs to 20000µs). Smaller values increase the processor load but lead to a smoother acceleration. A value of 5000µs is sufficient for most of real life applications. 
    parameter:
    - name: stepPulseWidth
      type: unsigned   
    - name: speedUpdatePeriod
      type: unsigned  

  - name: StepControl
    shortDesc: Constructs a StepControl object with default parameters for stepPulseWidth (5µs) and speedUpdatePeriod (5ms)

#----------------------------------------------------------------------------------------------------------------------------------------------------------
- name: Moving Motors to Targets
  anchor: moving
  methods:
  - name: moveAsync
    returnType: void   
    parameter:
    - name: "s1 ["
      type: Stepper&
    - name: "sN]"
      type: "...Stepper&"

    shortDesc: >
      Accepts up to N=10 references to stepper objects and starts moving them to their targets in sync. I.e. all motors will reach their targets at the same time. The function returns immediately after starting the movement (non blocking).

      ```c++                        
         StepControl controller;
         Stepper s1(1,2), s2(3,4);
         controller.moveAsync(s1,s2);        
      ```

  - name: moveAsync
    returnType: void   
    parameter:
    - name: "stepperArray[N]"
      type: Stepper*

    shortDesc: >-
      Accepts an array of up to N=10 pointers to stepper objects and starts moving them to their targets in sync. I.e. all motors will reach their targets at the same time. The function returns immediately after starting the movement (non blocking).
  
      ```c++                        
         StepControl controller;
         Stepper* stArray[] = { new Stepper(1,2), new Stepper(3,4), newStepper(5,6)};
         
         controller.moveAsync(stArray);        
      ```

  - name: move
    returnType: void   
    parameter:
    - name: "s1 ["
      type: Stepper&
    - name: "sN]"
      type: "...Stepper&"

    shortDesc: >
      Accepts up to N=10 references to stepper objects and starts moving them to their targets in sync. I.e. all motors will reach their targets at the same time. The function waits until the movement is finished (blocking).

      ```c++                        
         StepControl controller;
         Stepper s1(1,2), s2(3,4);
         controller.move(s1,s2);  // returns after the motors reached their targets   
      ```

  - name: move
    returnType: void   
    parameter:
    - name: "stepperArray[N]"
      type: Stepper*

    shortDesc: >-
      Accepts an array of up to N=10 pointers to stepper objects and starts moving them to their targets in sync. I.e. all motors will reach their targets at the same time. The function waits until the movement is finished (blocking).
  
      ```c++                        
         StepControl controller;
         Stepper* stArray[] = { new Stepper(1,2), new Stepper(3,4), newStepper(5,6)};
         
         controller.move(stArray); // returns after the motors reached their targets
      ```

#---------------------------------------------------------------------------------------------------------------------------------------------------------- 
- name: Stopping Motors
  anchor: stopping
  methods:
  - name: stopAsync
    shortDesc: Decelerates the controlled motors to a stop. This function returns immediately after starting the stop sequence (non blocking). 
    returnType: void   

  - name: stop
    shortDesc: Decelerates the controlled motors to a stop and waits until the motors are stopped (blocking function).
    returnType: void   

  - name: emergencyStop
    shortDesc: Immediately stops the controlled motors. Use this only in emergency since, depending on motor speed this will probably lead to step losses. A homing sequence is highly recommended after a call to emergencyStop().
    returnType: void   
---

Objects of the StepControl class are used to move up to 10 motors to target positions in sync. The synchronization is kept during acceleration and deceleration phases and all motors will reach their target positions at the same time. 
  



