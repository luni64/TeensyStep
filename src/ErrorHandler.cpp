#include "ErrorHandler.h"
#include <Arduino.h>

namespace TeensyStep
{
    errCallback_t* ErrorHandler::callback = nullptr;

    static Stream* stream;

   

    void vverboseHandler(int module, int code)
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
                    default:                  txt = "unknown"; break;
                }                
                break;

            case errModule::RB: //------------------------------------------
                mod = "RB";
                switch (code)
                {
                    case 0:
                        txt = "test";
                        break;

                    default:
                        txt = "asdf";
                        break;
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
        return vverboseHandler;
    }
}