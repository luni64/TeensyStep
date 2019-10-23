#pragma once
#include "Stream.h"

namespace TeensyStep
{
    using errCallback_t = void(int, int);

    enum class errModule;
    enum class pitERR;

    class ErrorHandler
    {
     public:
        static int error(errModule module, int code)
        {            
            if (callback != nullptr) callback((int)module, code);
            return code;
        }
        static void attachCallback(errCallback_t* cb) { callback = cb; }

     protected:
        static errCallback_t* callback;
    };

    extern errCallback_t* verboseHandler(Stream&);

    enum class errModule {
        PIT = 1,
        MC,
        RB 
    };

    enum class pitErr {
        OK,
        argErr,
        notAllocated,
        outOfTimers,
        
    };

    enum class mcErr {
        OK,
        alrdyMoving,        
    };
}