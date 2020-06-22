#include "ErrorHandler.h"
#include <Arduino.h>

namespace TeensyStep
{    
    static Stream* stream;   

    static void vHandler(int module, int code)
    {       
        const char* mod;
        const char* txt;

        switch ((errModule)module)
        {
            case errModule::PIT: //----------------------------------------
                mod = "PIT";
                switch ((pitErr)code)
                {
                    case pitErr::outOfTimers: txt = "No timer available"; break;
                    case pitErr::argErr:      txt = "BUG, argument error"; break;
                    case pitErr::notAllocated:txt = "BUG, timer not allocated"; break;
                    default:                  txt = "unknown"; break;
                }                
                break;           

            case errModule::MC: //------------------------------------------
                mod = "CTRL";
                switch ((mcErr)code)
                {
                    case mcErr::alrdyMoving:  txt = "Started while moving"; break;
                    default:                  txt = "unknown"; break;
                }                
                break;

            default: //------------------------------------------------------
                mod = "unknown";
                txt = "unknown";
                break;
        }
        stream->printf("ERR M%iC%i: %s (%s)\n", module, code, txt, mod);

        while (true)
        {
            digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
            delay(50);
        }
    }

    errCallback_t* verboseHandler(Stream& s)
    {
        stream = &s;
        return vHandler;
    }

    errCallback_t* ErrorHandler::callback = nullptr;
}