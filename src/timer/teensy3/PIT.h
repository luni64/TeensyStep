#pragma once

#include "IntervalTimer.h"

class IPitHandler
{
public:
	virtual void pitISR() = 0;
};

class TF_Handler;

class PIT
{
public:	
	bool begin(TF_Handler*);

	inline void end() { timer.end(); }
	inline void start() const { channel->TFLG = 1; channel->TCTRL = 0; channel->TCTRL = 3; }
	inline void stop() const { channel->TCTRL &= ~PIT_TCTRL_TIE; }	
	inline void enableInterupt() const { channel->TFLG = 1; channel->TCTRL |= PIT_TCTRL_TIE; }
	inline void setFrequency(uint32_t val) const {  channel->LDVAL = F_BUS/val;}
	inline void setThisReload(uint32_t ldval) const { channel->TCTRL = 0, channel->TFLG = 1;channel->LDVAL = ldval, channel->TCTRL = 3;}
	inline void setNextReload(uint32_t ldval) const { channel->LDVAL = ldval; }
	inline uint32_t getLDVAL() const { return channel->LDVAL; }
	inline uint32_t getCVAL() const { return channel->CVAL; }
	inline void clearTIF()const { channel->TFLG = 1; }
	inline bool isRunning() const { return channel->TCTRL & PIT_TCTRL_TIE; }
	
	KINETISK_PIT_CHANNEL_t* channel = nullptr;

  protected:	
	IntervalTimer timer;
	void setupChannel();

};