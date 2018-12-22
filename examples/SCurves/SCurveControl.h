#include <StepControlBase.h>

class SCurveControl : public StepControlBase<>
{
  protected:
    // Defaults
    uint32_t speedMax; // steps/s
    uint32_t speedMin; // steps/s
    uint32_t currentSpeed;
    uint32_t accelMax; // steps/s^2
    uint32_t accelAvg;
    uint32_t distance;
    uint32_t dv, d1, d2, d3;
    float v1, v2;
    float dt, t1, t2, t3;
    
    uint32_t jerkMax, jerkMaxHalf, jerkRoot;

    inline uint32_t prepareMovement(uint32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t acceleration);
    inline uint32_t updateSpeed(uint32_t currentPosition);
    inline uint32_t initiateStopping(uint32_t currentPosition);
};

uint32_t SCurveControl::prepareMovement(uint32_t targetPos, uint32_t targetSpeed, uint32_t pullInSpeed, uint32_t a)
{
    distance = targetPos;
    speedMax = targetSpeed;
    speedMin = pullInSpeed;
    
    // This creates a "pure" s-curve with no linear segment
    accelAvg = a;
    accelMax = a*2; 

    dv = speedMax-speedMin; 
    t3 = (float) dv / (float) accelAvg; // Time to reach max speed
    float d = (dv*t3)/2.0f;
    
    // Determine if there's room to reach the max speed
    // if not change the max speed to be the max that can be reached
    if (d > distance/2) {
        dv = sqrtf(accelAvg*distance);
        t3 = (float) dv/ (float) accelAvg;
    }
    
    // Compute s-curve parameters
    uint32_t da = accelMax-accelAvg;
    jerkMax = (float) accelMax*accelMax*accelAvg/((float) dv*da);
    jerkMaxHalf = jerkMax/2.0f;
    jerkRoot = powf(6.0f/jerkMax, 1.0f/3.0f);
    
    t1 = accelMax/jerkMax;
    t2 = std::max(0.0f, t3-t1);
    v1 = jerkMax*t1*t1/2.0f;
    v2 = dv-v1;
    dt = t2-t1;
    d3 = dv*dv/(2*accelAvg);
    
    // On short distances we have to clip d2 and d1 to be less than d3
    // because d3 is the only value that is dependent on the speed limit
    d1 = std::min(d3, uint32_t (jerkMax*t1*t1*t1/6));
    d2 = std::min(d3, uint32_t (d1 + accelMax*dt*dt/2 + v1*dt));
    currentSpeed = speedMin;
    return speedMin;
}

uint32_t SCurveControl::updateSpeed(uint32_t currentPosition)
{
        uint32_t pos = currentPosition;    
        uint32_t e = distance-pos;
        float t;
        uint32_t v;
        
        // Accel stages
        if (pos <= d1) { // Stage 1
            t = jerkRoot*powf(pos, 1.0f/3.0f);
            v = jerkMaxHalf*t*t;
        } else if (pos <= d2) { // Stage 2
            pos -= d1;
            t = std::max(0.0f, (sqrtf(v1*v1+2*accelMax*pos)-v1)/accelMax);
            v = accelMax*t + v1;
        } else if (pos <= d3) { // Stage 3
            t = t3-jerkRoot*powf(pos, 1.0f/3.0f);
            v = dv-jerkMaxHalf*t*t;
        
        // Decel stages
        } else if (e <= d1) { // Stage 7
            t = jerkRoot*powf(pos, 1.0f/3.0f);
            v = jerkMaxHalf*t*t;
        } else if (e <= d2) { // Stage 6
            e -= d1;
            t = std::max(0.0f, (sqrtf(v1*v1+2*accelMax*e)-v1)/accelMax);
            v = accelMax*t + v1;
        } else if (e <= d3) { // Stage 5
            t = t3-jerkRoot*powf(pos, 1.0f/3.0f);
            v = dv-jerkMaxHalf*t*t;
        } else {
            // Stay at the same speed
            return currentSpeed;
        }
        
        // Clip just to be safe
        currentSpeed = std::min(speedMax, std::max(speedMin, v));
        return currentSpeed;
}

uint32_t SCurveControl::initiateStopping(uint32_t s_cur)
{
    uint32_t e = distance-s_cur;
    if (e <= d3)  // we are already decelerationg, nothing to change...
    {
        return e;
    }
    else                             // accelerating or constant speed phase
    {
        // TODO: This will create jerk if accelerating
        uint32_t newTarget = e-d3;
        return newTarget;
    }
}
