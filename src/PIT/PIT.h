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

	inline void stop() { channel->TCTRL = 0; }
	inline void start() { channel->TCTRL = 0; channel->TCTRL = 3; }
	inline void setCounter(uint32_t val) { channel->CVAL = val; }
	inline void enableInterupt() { channel->TFLG = 1; channel->TCTRL |= PIT_TCTRL_TIE; }
	inline void disableInterupt() { channel->TCTRL &= ~PIT_TCTRL_TIE; }
	inline void setReloadValue(uint32_t val) { channel->TCTRL = 0; channel->LDVAL = val; channel->TCTRL = 3; }  // also starts timer!
	inline void clearTIF() { channel->TFLG = 1; }
	inline bool isRunning() { return channel->TCTRL & PIT_TCTRL_TIE; }

protected:
public:
	IntervalTimer timer;
	void setupChannel();

	KINETISK_PIT_CHANNEL_t* channel = nullptr;
};
