#pragma once

struct TF_Handler
{
  virtual void stepTimerISR() = 0;
  virtual void accTimerISR() = 0;
  virtual void pulseTimerISR() = 0;
};