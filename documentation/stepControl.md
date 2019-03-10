---
layout:         mypage

className: StepControl
includeFile:  TeensyStep.h

chapters:
- name: Constructor
  anchor: constructor
  methods:
  - name: StepControl
    shortDesc: Pulsewidth and updatePeriod in µs.
    parameter:
    - name: pulseWidth
      type: unsigned   
    - name: updatePeriod
      type: unsigned      
      
- name: Public Function Members
  anchor: publicMethods
  methods:

  - name: move
    shortDesc: Takes up to 10 Stepper objects and moves them to their target positions.
    returnType: void   
    parameter:
    - name: "m1 ["
      type: Stepper&
    - name: "m10]"
      type: "...Stepper&"

  - name: move
    shortDesc: Takes an array of up to 10 pointers to Stepper objects and moves them to their target positions.
    returnType: void   
    parameter:
    - name: motors
      type: Stepper*    

  - name: stop    
    shortDesc: Starts a stop sequence and blocks until the motors are stopped
    returnType: void   
  
  - name: moveAsync
    shortDesc: Takes up to 10 Stepper objects and moves them to their target positions (non blocking).
    returnType: void   
    parameter:
    - name: "m1 ["
      type: Stepper&
    - name: "m10]"
      type: "...Stepper&"

  - name: moveAsync
    shortDesc: Takes an array of up to 10 pointers to Stepper objects and moves them to their target positions  (non blocking).
    returnType: void   
    parameter:
    - name: motors
      type: Stepper*    

  - name: stopAsync
    shortDesc: Starts a stop sequence and blocks until the motors are stopped  (non blocking).
    returnType: void   
  




    
---

asfasdfsdf


  



