#include <ctime>

#include "core/utils/time.h"
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
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return timespecToMsec(&now);
}

bool setTimeMsec(uint64_t timestamp)
{
    return false;
}

}
