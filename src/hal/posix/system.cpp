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

/** Time when the system was booted at */
static struct timespec lastBootAt;

/** Flag to prevent the delivery of the next reset signal in unit tests */
static bool shouldPreventNextReset = false;

/** Number of reset attempts that were prevented */
static size_t numResetsPrevented = 0;

namespace teller::hal::system {

void init()
{
    int retval = clock_gettime(CLOCK_MONOTONIC, &lastBootAt);
    assert(retval == 0);

    numResetsPrevented = 0;
    shouldPreventNextReset = false;
}

void destroy()
{
    shouldPreventNextReset = false;
    numResetsPrevented = 0;
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
    if (shouldPreventNextReset) {
        shouldPreventNextReset = false;
        numResetsPrevented++;
    } else {
        kill(getpid(), SIGUSR1);
    }
}
/* LCOV_EXCL_STOP */

void preventNextReset()
{
    shouldPreventNextReset = true;
}

size_t countPreventedResetAttempts(void)
{
    return numResetsPrevented;
}

}
