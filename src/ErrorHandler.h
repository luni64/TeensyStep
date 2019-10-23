#pragma once
#include "Arduino.h"

namespace TeensyStep
{
    using errCallback_t = void(int, int);

    class ErrorHandler
    {
     public:
        static int error(int module, int code)
        {
            digitalWriteFast(13,HIGH);
            if (callback != nullptr) callback(module, code);
            return code;
        }
        static void attachCallback(errCallback_t* cb) { callback = cb; }

     protected:
        static errCallback_t* callback;
    };
}