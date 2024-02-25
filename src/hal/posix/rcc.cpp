#include "hal/rcc.h"

namespace teller::hal::rcc {

bool init()
{
    return true;
}

void destroy()
{
}

reset_reason_t getReasonOfLastReset(void)
{
    return RESET_REASON_NORMAL;
}

}
