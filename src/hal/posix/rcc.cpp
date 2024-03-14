#include <signal.h>
#include <unistd.h>

#include "hal/rcc.h"

namespace teller::hal::rcc {

static bool resetRequested = false;
static reset_reason_t reasonOfLastReset = RESET_REASON_NORMAL;

bool init()
{
    reasonOfLastReset = resetRequested ? RESET_REASON_SOFTWARE : RESET_REASON_NORMAL;
    resetRequested = false;
    return true;
}

void destroy()
{
}

reset_reason_t getReasonOfLastReset(void)
{
    return reasonOfLastReset;
}

/* LCOV_EXCL_START */
void requestReset()
{
    kill(getpid(), SIGUSR1);
}
/* LCOV_EXCL_STOP */

}
