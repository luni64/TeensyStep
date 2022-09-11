#ifndef _ERRORHANDLER_H__
#define _ERRORHANDLER_H__

#include <stddef.h>

typedef void(*errCallback_t)(int, int);

typedef enum {
    eM_PIT = 1,
    eM_MC,
    eM_RB
}errModule;

typedef enum {
    pE_OK,
    pE_argErr,
    pE_notAllocated,
    pE_outOfTimers,

}pitErr;

typedef enum {
    mE_OK,
    mE_alrdyMoving,
}mcErr;


int error(errModule module, int code);

void attachCallback(errCallback_t cb);

void vHandler(int module, int code);

#ifdef USE_FULL_ASSERT

#else
#define ASSERT(x)     (void)(x)
#endif
#endif
