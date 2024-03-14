#include <signal.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <ctime>
#include <thread>

#include "core/utils/time.h"
#include "hal/system.h"

using namespace std;
using namespace teller::hal::system;

static struct timespec lastBootAt;

namespace teller::hal::system {

void init()
{
    int retval = clock_gettime(CLOCK_MONOTONIC, &lastBootAt);
    assert(retval == 0);
}

void destroy()
{
}

std::uint32_t getTimeSinceBootMsec(void)
{
    struct timespec now, diff;

    int retval = clock_gettime(CLOCK_MONOTONIC, &now);
    assert(retval == 0);

    timespecDiff(&lastBootAt, &now, &diff);

    return diff.tv_sec * 1000 + diff.tv_nsec / 1000000;
}

void delayMsec(uint32_t delay)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
}

/* LCOV_EXCL_START */
void requestReset()
{
    kill(getpid(), SIGUSR1);
}
/* LCOV_EXCL_STOP */

}
