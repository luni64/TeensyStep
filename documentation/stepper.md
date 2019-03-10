---
layout:        mypage

className:   Stepper
includeFile: TeensyStep.h

chapters: 
  - name: Constructor
    anchor: constructor
    methods:
    - name: Stepper
      shortDesc: Constructs a stepper object with the given pin numbers for STEP and DIR signals     
      parameter:
        - name: stpPin
          type: unsigned
        - name: dirPin
          type: unsigned
  
  - name: Public Member Functions
    anchor: public-member-functions
    methods:
    - name: setMaxSpeed
      shortDesc: Sets the maximum speed of the motor in septs/s.
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
            
    - name: setPullInSpeed
      shortDesc: Sets the pull in speed in steps/s.
      returnType: Stepper&
      parameter:
        - name: speed
          type: uint32_t

    - name: setTargetAbs
      returnType: Stepper&
      parameter:
        - name: pos
          type: int32_t
      shortDesc: Sets the target position of the next movement. 

    - name: setTargetRel
      shortDesc: Sets the target position of the next movement relative to current position [steps]
      returnType: Stepper&
      parameter:
        - name: delta
          type: int32_t
---

UNDER CONSTRUCTION

  



