#pragma once

// TEENSYSTEP_HOT marks the functions that run for every step. On Teensy 3.5 and 3.6 they are placed in RAM:
// at 120 and 180 MHz the processor reads code from the flash memory with wait states, and code in RAM runs
// about twice as fast. On the other boards the mark does nothing (a Teensy 4 runs all code from RAM).

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define TEENSYSTEP_HOT __attribute__((section(".fastrun")))
#else
#define TEENSYSTEP_HOT
#endif
