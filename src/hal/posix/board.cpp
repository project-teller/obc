#include "hal/board.h"

namespace teller::hal::board {

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

float getBoardTemperature(void)
{
    return 25.0f;
}

float getBoardVoltage(void)
{
    return 3.3f;
}

}
