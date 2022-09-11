#include "ErrorHandler.h"


static errCallback_t callback = NULL;



int error(errModule module, int code){
    if(callback != NULL) callback((int)module, code);
    return code;
}

void attachCallback(errCallback_t cb){
    callback = cb;
}

void vHandler(int module, int code)
{
    const char* mod;
    const char* txt;

    switch ((errModule)module)
    {
        case eM_PIT: //----------------------------------------
            mod = "PIT";
            switch ((pitErr)code)
            {
                case pE_outOfTimers: txt = "No timer available"; break;
                case pE_argErr:      txt = "BUG, argument error"; break;
                case pE_notAllocated:txt = "BUG, timer not allocated"; break;
                default:                  txt = "unknown"; break;
            }
            break;

        case eM_MC: //------------------------------------------
            mod = "CTRL";
            switch ((mcErr)code)
            {
                case mE_alrdyMoving:  txt = "Started while moving"; break;
                default:                  txt = "unknown"; break;
            }
            break;

        default: //------------------------------------------------------
            mod = "unknown";
            txt = "unknown";
            break;
    }
    // stream->printf("ERR M%iC%i: %s (%s)\n", module, code, txt, mod);
    (void)mod;
    (void)txt;
    while (1)
    {
        // digitalToggle(LED_BUILTIN);
        // delay(50);
    }
}
