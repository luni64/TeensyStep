#include "Winder.h"

Winder::Winder(Stepper &_spindle, Stepper &_feeder)
    : spindle(_spindle), feeder(_feeder), feederCtrl(5, 1000), spindleCtrl(5, 1000)
{    
}

Winder& Winder::setSpindleParams(unsigned stepsPerRev, unsigned acc)
{
    spindleStpPerRev = stepsPerRev;
    spindleAcc =  acc;

    spindle.setMaxSpeed(1);   // -> parameter of overrideSpeed equals real speed
    spindle.setAcceleration(spindleAcc);

    oldSpindleSpeed = 0.0f;
    oldPitch = 1.0f;
   
    return *this;
}

Winder& Winder::setFeederParams(unsigned stpPerMM, unsigned acc)
{
    feederStpPerMM = stpPerMM;
    feederAcc = acc;

    feeder.setMaxSpeed(1); 
    feeder.setAcceleration(spindleAcc);

    return *this;
}

void Winder::begin() 
{
    pitchFactor = (float)feederStpPerMM / spindleStpPerRev;
    
    // startup controllers   
    feederCtrl.rotateAsync(feeder);
    feederCtrl.overrideSpeed(0.0f);
    
    spindleCtrl.rotateAsync(spindle);
    spindleCtrl.overrideSpeed(0.0f);   
}

void Winder::setSpindleSpeed(float rpm)
{
    targetSpindleSpeed = rpm / 60.0f * spindleStpPerRev;
}

void Winder::setPitch(float pitch)
{
    if(pitch > 0) targetPitch = pitch;
}

void Winder::updateSpeeds()
{   
    float targetFeederSpeed =  targetPitch * pitchFactor * targetSpindleSpeed;

    if(targetSpindleSpeed != oldSpindleSpeed)  // if target speed changed -> update all
    {
        oldSpindleSpeed = targetSpindleSpeed;

        float feederAccFactor = targetPitch * pitchFactor;

        //Serial.println(feederAccFactor);

        feederCtrl.overrideAcceleration(feederAccFactor);
        feederCtrl.overrideSpeed(targetFeederSpeed);
        spindleCtrl.overrideSpeed(targetSpindleSpeed);
    }
    else if(targetPitch != oldPitch)           // if only target pitch changed, update feeder speed only
    {        
        oldPitch = targetPitch;
        feederCtrl.overrideAcceleration(feederAcc / spindleAcc);
        feederCtrl.overrideSpeed(targetFeederSpeed);
    }
}