

#ifndef _TF_HANDLER_H__
#define _TF_HANDLER_H__

typedef void (*timrISR)(void *ctx);

typedef struct {
    timrISR stepTimerISR;
    timrISR accTimerISR;
    timrISR pulseTimerISR;
}TF_Handler;


#endif
