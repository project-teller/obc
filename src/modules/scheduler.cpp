#include "modules/scheduler.h"
#include "hal/system.h"

static uint32_t startedAt = 0;
static uint32_t stoppedAt = 0;
static bool isClockRunning = false;

namespace teller::scheduler {

bool init(void)
{
    stop();
    reset();
    return true;
}

void destroy(void)
{
    stop();
    reset();
}

uint32_t getElapsedTimeMsec(uint32_t now)
{
    now = now ? now : teller::hal::system::getTimeSinceBootMsec();
    return isClockRunning ? now - startedAt : stoppedAt - startedAt;
}

bool isRunning(void)
{
    return isClockRunning;
}

void reset(void)
{
    startedAt = isClockRunning ? teller::hal::system::getTimeSinceBootMsec() : 0;
    stoppedAt = 0;
}

void start(void)
{
    startedAt = teller::hal::system::getTimeSinceBootMsec();
    stoppedAt = 0;
    isClockRunning = true;
}

void stop(void)
{
    if (isClockRunning) {
        stoppedAt = teller::hal::system::getTimeSinceBootMsec();
        isClockRunning = false;
    }
}

}
