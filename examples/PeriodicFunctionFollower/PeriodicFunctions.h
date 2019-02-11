#pragma once

#include "TeensyStep.h"
#include <cmath>

constexpr float twoPi = 2 * M_PI;

class RoseFunction
{
  public:
    RoseFunction(PathStepper &spindle, PathStepper &slide)
    {
        spindle.attachFunction([this](float t) { return spindleFunction(t); });
        spindle.setScale(1000);

        slide.attachFunction([this](float t) { return slideFunction(t); });
        slide.setScale(1000);
    }

    inline float spindleFunction(float t)
    {        
        theta = omega * t; // constant revolution
        //Serial.printf("%f %f\n",t, theta);
        return theta;
    }

    inline float slideFunction(float t)
    {
        float phi = fmodf(k * theta, twoPi);
        return cos(phi);
    }

    inline RoseFunction &setParameters(unsigned n, unsigned d)
    {
        k = (float)n / d;
        return *this;
    }

    inline RoseFunction &setRPM(float rpm)
    {
        omega = rpm * twoPi / 60.0f; // circular frequency in rad/s
        return *this;
    }

  protected:
    float k;
    float theta;
    float omega = twoPi / 60.0f; // default 1rpm
};