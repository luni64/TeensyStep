#include "Arduino.h"
#include "TeensyStep.h"

//spindle settings
constexpr unsigned rpm = 15;
constexpr unsigned spindleSPR = 6400;                    // stp per rev
constexpr unsigned spindleSpeed = rpm * spindleSPR / 60; // stp/sec
constexpr unsigned spindleStepPin = 1;
constexpr unsigned spindleDirPin = 2;

//slide settings
constexpr unsigned slideAmplitude = 10000; // stp
constexpr unsigned slideSpeed = 35000;     //stp/sec
constexpr unsigned slideStepPin = 3;
constexpr unsigned slideDirPin = 4;

//accuracy
IntervalTimer tickTimer;
constexpr unsigned recalcPeriod = 25'000; //µs  period for calculation of new target points. Straight lines between those points.
constexpr float dt = recalcPeriod / 1E6;  // seconds

// rose fuction
constexpr int n = 5;
constexpr int d = 8;
constexpr float k = (float)n / d;

float slideFunc(float spindleAngle)
{
  float phi = fmodf(spindleAngle * k, TWO_PI);
  return slideAmplitude * cosf(phi);
}

// TeensyStep
RotateControl slideController;
RotateControl spindleController;

Stepper spindle(spindleStepPin, spindleDirPin);
Stepper slide(slideStepPin, slideDirPin);


//------------------------------------------------------------------------------------
// tick()
//
// This function is called periodically with period recalcPeriod. 
// It calculates 
//  1) a new target value for the slide depending on the spindle angle
//  2) the new speed for the spindle so that it will reach the target until it is called again

void tick()
{
  float spindleAngle = spindle.getPosition() * (TWO_PI / spindleSPR); //convert steps to angle
  float slidePosition = slide.getPosition();
  float slideTarget = slideFunc(spindleAngle);

  float newSpeed = (slideTarget - slidePosition) / dt; // speed to reach target in given delta t (neglecting acceleration)
  float speedFac = newSpeed / slideSpeed;              // transform in relative factor (-1.0 .. 1.0)

  slideController.overrideSpeed(speedFac);             // set new speed
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
 
  while (!Serial && millis() < 1000);

  spindle
      .setMaxSpeed(spindleSpeed)
      .setAcceleration(50000);

  slide
      .setMaxSpeed(slideSpeed)
      .setAcceleration(150000)
      .setPosition(slideFunc(0)); // set start position of counter

  spindleController.rotateAsync(spindle); // let the spindle run with constant speed
  slideController.rotateAsync(slide);
  slideController.overrideSpeed(0); // start with stopped slide
  tick();

  // use a timer to periodically calculate new targets for the slide
  tickTimer.priority(255); // lowest priority, potentially long caclulations need to be interruptable by TeensyStep
  tickTimer.begin(tick, recalcPeriod);
}

void loop()
{
  digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));

  // print current values of spindle angle [rad] and slide position [steps]
  float phi = spindle.getPosition() * (TWO_PI / spindleSPR);
  int32_t r = slide.getPosition();

  Serial.printf("%f\t%d\n", phi, r);

  delay(20);
}
