#pragma once

#include <kinetis.h>
#include <core_pins.h>

namespace TeensyStepFTM
{


//==========================================================================
// Available timer modules for the Teensy XX boards. Please note that those
// timers are also used by the core libraries for PWM and AnalogWrite.
// Therefore, choose a timer which isn't attached to the pins you need for
// PWM or AnalogWrite. (TEENSY LC not yet supported)
//
// D: Default, X: available
//
//      TIMER           | Channels|        Timer is available for           |
//                      |         | T-LC | T3.0 |  T3.1 | T3.2 | T3.5 | T3.6|
//----------------------|---------|------|------|-------|------|------|-----|
#define TIMER_FTM0 1  //|   8     |      |  D   |   D   |  D   |  D   |  D  |
#define TIMER_FTM1 2  //|   2     |      |  X   |   X   |  X   |  X   |  X  |
#define TIMER_FTM2 3  //|   2     |      |      |   X   |  X   |  X   |  X  |
#define TIMER_FTM3 4  //|   8     |      |      |       |      |  X   |  X  |
#define TIMER_TPM0 5  //|   6     |      |      |       |      |      |     |
#define TIMER_TPM1 6  //|   2     |      |      |       |      |      |  X  |
#define TIMER_TPM2 7  //|   2     |      |      |       |      |      |  X  |
#define TIMER_DEFAULT -1

// If you need a special timer, please replace "TIMER_DEFAULT" by a timer from the list above
#define USE_TIMER TIMER_DEFAULT


//==========================================================================
// Nothing to be changed below here
//==========================================================================



#if USE_TIMER == TIMER_DEFAULT
#undef USE_TIMER
#if defined __MKL26Z64__
#define USE_TIMER TIMER_TPM0
#else
#define USE_TIMER TIMER_FTM0
#endif
#endif

    constexpr unsigned selTimer = USE_TIMER - 1;
    constexpr bool isFTM = selTimer <= 3;

    //================================================
    // Which board do we use

    enum class _boards
    {
        T_LC, T_30, T31_2, T_35, T_36
    };

#if defined __MKL26Z64__
    constexpr _boards board = _boards::T_LC;
#elif defined __MK20DX128__
    constexpr _boards board = _boards::T_30;
#elif defined __MK20DX256__
    constexpr _boards board = _boards::T31_2;
#elif defined __MK64FX512__
    constexpr _boards board = _boards::T_35;
#elif defined __MK66FX1M0__
    constexpr _boards board = _boards::T_36;
#endif


    //====================================================================
    // Memory layout of register banks for FTM and TPM timers

    typedef struct      // FTM & TPM Channels
    {
        uint32_t SC;
        uint32_t CV;
    } FTM_CH_t;

    typedef struct       // FTM register block (this layout is compatible to a TPM register block)
    {
        uint32_t SC;
        uint32_t CNT;
        uint32_t MOD;
        FTM_CH_t CH[8];
        uint32_t CNTIN;
        uint32_t STATUS;
        uint32_t MODE;
        uint32_t SYNC;
        uint32_t OUTINIT;
        uint32_t OUTMASK;
        uint32_t COMBINE;
        uint32_t DEADTIME;
        uint32_t EXTTRIG;
        uint32_t POL;
        uint32_t FMS;
        uint32_t FILTER;
        uint32_t FLTCTRL;
        uint32_t QDCTRL;
        uint32_t CONF;
        uint32_t FLTPOL;
        uint32_t SYNCONF;
        uint32_t INVCTRL;
        uint32_t SWOCTRL;
        uint32_t PWMLOAD;
    }FTM_t;

    //-----------------------------------------------------------------------------------
    // Definition of base address for register block, number of channels and IRQ number
    //

    constexpr uintptr_t TimerBaseAddr[][7] =
    { //    FTM0         FTM1        FTM2        FTM3        TPM0        TMP1        TMP2
        { 0,          0,          0,          0,          0,          0,          0          },  // Teensy LC not yet supported
        { 0x40038000, 0x40039000, 0,          0,          0,          0,          0,         },  // Teensy 3.0
        { 0x40038000, 0x40039000, 0x400B8000, 0,          0,          0,          0,         },  // Teensy 3.1/3.2
        { 0x40038000, 0x40039000, 0x400B8000, 0x400B9000, 0,          0,          0,         },  // Teensy 3.5
        { 0x40038000, 0x40039000, 0x400B8000, 0x400B9000, 0,          0x400C9000, 0x400CA000 },  // Teensy 3.6
    };

    constexpr int IRQ_Number[][7]
    {
        //  FTM0  FTM1  FTM2  FTM3  TPM0  TPM1  TPM2
          {  0,    0,   0,     0,    0,    0,    0 },  // Teensy LC
          { 25,   26,   0,     0,    0,    0,    0 },  // Teensy 3.0
          { 62,   63,   64,    0,    0,    0,    0 },  // Teensy 3.1/3.2
          { 42,   43,   44,   71,    0,    0,    0 },  // Teensy 3.5
          { 42,   43,   44,   71,    0,   88,   89 },  // Teensy 3.6
    };

    constexpr int _nrOfChannels[]
    {
        8,  // FTM0
        2,  // FTM1
        2,  // FTM2
        8,  // FTM3
        6,  // TPM0
        2,  // TPM1
        2,  // TPM2
    };

    constexpr uintptr_t timerAddr = TimerBaseAddr[(int)board][selTimer];
    volatile FTM_t *const timer = (FTM_t *)timerAddr;                   // base address for register block of selected timer
    constexpr unsigned irq = IRQ_Number[(int)board][selTimer];          // IRQ number of selected timer
    constexpr unsigned maxChannel = _nrOfChannels[selTimer];            // Number of channels for selected timer

    static_assert(timerAddr != 0 && irq != 0, "Board does not support choosen timer");  //Generate compiler error in case the board does not support the selected timer

    //-----------------------------------------------------------------------------------
    //Frequency dependent settings

    constexpr unsigned _timer_frequency = isFTM ? F_BUS : 16000000;  // FTM timers are clocked with F_BUS, the TPM timers are clocked with OSCERCLK (16MHz for all teensies)

    // Choose prescaler such that one timer cycle corresponds to about 1µs
    constexpr unsigned prescale =
        _timer_frequency > 120000000 ? 0b111 :
        _timer_frequency > 60000000 ? 0b110 :
        _timer_frequency > 30000000 ? 0b101 :
        _timer_frequency > 15000000 ? 0b100 :
        _timer_frequency > 8000000 ? 0b011 :
        _timer_frequency > 4000000 ? 0b010 :
        _timer_frequency > 2000000 ? 0b001 : 0b000;

    // Calculates required reload value to get a delay of mu microseconds.
    // this will be completely evaluated by the compiler as long as mu is known at compile time
    constexpr int microsToReload(const float mu)
    {
        return  mu * 1E-6 * _timer_frequency / (1 << prescale) + 0.5;
    }
}
