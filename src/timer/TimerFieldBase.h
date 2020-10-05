#include "TF_Handler.h"

class ITimerField
{
public:
  ITimerField(TeensyStep::TF_Handler *);

  virtual bool begin()=0;
  virtual void end()=0;

  virtual void stepTimerStart()=0;
  virtual void stepTimerStop()=0;
  virtual bool stepTimerIsRunning() const =0;
  virtual void setStepFrequency(unsigned f)=0;

  virtual void accTimerStart()=0;
  virtual void accTimerStop()=0;
  virtual void setAccUpdatePeriod(unsigned period)=0;

  virtual void setPulseWidth(unsigned delay)=0;
  virtual void triggerDelay()=0;
};
