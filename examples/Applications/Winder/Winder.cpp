#include "winder.h"

Winder::Winder(Stepper &_spindle, Stepper &_feeder)
    : spindle(_spindle), feeder(_feeder), spindleCtrl(5, 1000), feederCtrl(5, 1000)
{}

Winder& Winder::setSpindleParams(unsigned stepsPerRev, unsigned acc)
{
    spindleStpPerRev = stepsPerRev;
    acceleration = acc;   
    return *this;
}

Winder& Winder::setFeederParams(unsigned stpPerMM)
{
    feederStpPerMM = stpPerMM;  
    return *this;
}

void Winder::begin() 
{
    pitchFactor = (float)feederStpPerMM / spindleStpPerRev;
    
    // startup controllers
    feeder.setMaxSpeed(1);  // setting to 1 -> parameter of overrideSpeed equals real speed
    feeder.setAcceleration(acceleration);
    feederCtrl.rotateAsync(feeder);
    feederCtrl.overrideSpeed(0.0f);

    spindle.setMaxSpeed(1);
    spindle.setAcceleration(acceleration);
    spindleCtrl.rotateAsync(spindle);
    spindleCtrl.overrideSpeed(0.0f);
}

void Winder::setSpindleSpeed(float rpm)
{
    targetSpindleSpeed = rpm / 60.0f * spindleStpPerRev;
}

void Winder::setPitch(float pitch)
{
    targetPitch = pitch;
}

void Winder::updateSpeeds()
{
    float accFactor = pitchFactor * targetPitch;
    float targetFeederSpeed = accFactor * targetSpindleSpeed;

    feederCtrl.overrideAcceleration(accFactor);
    feederCtrl.overrideSpeed(targetFeederSpeed);
    spindleCtrl.overrideSpeed(targetSpindleSpeed);
}