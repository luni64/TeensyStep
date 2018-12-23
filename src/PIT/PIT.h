#pragma once

#include "IntervalTimer.h"

class IPitHandler
{
public:
	virtual void pitISR() = 0;
};

class PIT
{
public:
	bool begin(IPitHandler* handler);

	inline void end() const { channel->TCTRL = 0; }
	inline void start() const { channel->TFLG = 1; channel->TCTRL = 0; channel->TCTRL = 3; }
	inline void stop() const { channel->TCTRL &= ~PIT_TCTRL_TIE; }
	inline void setCounter(uint32_t val) const { channel->CVAL = val; }
	inline void enableInterupt() const { channel->TFLG = 1; channel->TCTRL |= PIT_TCTRL_TIE; }
	inline void setFrequency(uint32_t val) const {  channel->LDVAL = F_BUS/val;}  // also starts timer!
	inline void clearTIF()const { channel->TFLG = 1; }
	inline bool isRunning() const { return channel->TCTRL & PIT_TCTRL_TIE; }

protected:
public:
	IntervalTimer timer;
	void setupChannel();

	KINETISK_PIT_CHANNEL_t* channel = nullptr;
};