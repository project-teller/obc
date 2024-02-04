#include "hal/rtc.h"

namespace teller::hal::rtc {

bool init()
{
    return true;
}

void destroy()
{
}

uint64_t getTimeMsec()
{
    /* TODO */
    return 0;
}

bool setTimeMsec(uint64_t timestamp)
{
    return false;
}

}
