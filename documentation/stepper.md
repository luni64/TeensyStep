---
layout:      mypage

className:   Stepper
includeFile: TeensyStep.h

chapters: 

#==============================================================================================

  - name: Construction
    anchor: constructor
    methods:
    - name: Stepper
      shortDesc: Constructs a stepper object with the given pin numbers for STEP and DIR signals     
      parameter:
        - name: stpPin
          type: unsigned
        - name: dirPin
          type: unsigned
  
  - name: Setup Functions
    anchor: setupFunctions
    methods:
    - name: setMaxSpeed
      shortDesc: Sets the maximum speed of the motor in steps/s. 
      longDesc: long description. asdf afas flasjf lasfj löasf jlöasfjk 
      returnType: Stepper&
      parameter:
        - name: speed
          type: int32_t

    - name: setAcceleration    
      shortDesc: Sets the maximum acceleration of the motor in steps/s².
      returnType: Stepper&
      parameter: 
        - name: accel
          type: uint32_t
            
    - name: setStepPinPolarity
      shortDesc: Sets the polarity of the generated step pulses. ```setStepPinPolarity(HIGH)``` generates active high pulses. ```setStepPinPolarity(LOW)``` generates active low pulses. 
      returnType: Stepper&
      parameter:
        - name: polarity
          type: int

    - name: setInversRotation
      shortDesc: Use this function to change the polarity of the generated direction signal. ```setInversRotation(true)``` sets the DIR pin to LOW if the motor runs in 'upward direction'
      returnType: Stepper&
      parameter:
        - name: polarity
          type: int

    summary: >-     

      ```c++ 
       // you can use the standard interface for setup
       motor_1.setAcceleration(5000);
       motor_1.setMaxSpeed(10000);       
      
       // ...or a fluent interface
       motor_2
        .setAcceleration(20000)
        .setMaxSpeed(15000)
        .setInverseRotation(true)
        .setStepPinPolarity(LOW);
      ```

#==============================================================================================

  - name: Motor Positioning
    anchor: positioning-functions
    methods:
    - name: setTargetAbs
      shortDesc: Sets the target position in steps. To actually move the motor, use one of the controller objects
      longDesc: 
      returnType: void
      parameter:
        - name: position
          type: int32_t

    - name: setTargetRel
      shortDesc: Sets the target position relative to the current position. To actually move the motor, use one of the controller objects
      longDesc: 
      returnType: void
      parameter:
       - name: delta
         type: int32_t

    - name: setPosition
      shortDesc: >
        Sets the internal step counter to the parameter ```counterValue```. 
        This function is NOT setting a new target position. 
        Typically, you only use this function after homing to set the counter to zero or some offset value.       
      returnType: void
      parameter:
      - name: counterValue 
        type: int32_t

    - name: getPosition
      shortDesc: Returns the current position of the stepper. 
      longDesc: 
      returnType: int32_t
     
---

Objects of the Stepper class store the physical properties of a stepper motor and the pins it is connected to. The target position of a a stepper object is used by the StepControl class to actually move the motor.  Stepper objects also maintain a step counter which reflects the current motor position. Writing to the step counter is possible to set the counter to zero (or some offset) at the home position. 


