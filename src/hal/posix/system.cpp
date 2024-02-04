#include <cassert>
#include <ctime>

#include "core/utils/time.h"
#include "hal/system.h"

using namespace std;
using namespace teller::hal::system;

struct timespec lastBootAt;

reset_reason_t reasonOfLastReset = RESET_REASON_UNKNOWN;

void teller::hal::system::init()
{
    int retval = clock_gettime(CLOCK_MONOTONIC, &lastBootAt);
    assert(retval == 0);
}

void teller::hal::system::destroy()
{
}

reset_reason_t teller::hal::system::getReasonOfLastReset(void)
{
    return RESET_REASON_NORMAL;
}

std::uint32_t teller::hal::system::getTimeSinceBootMsec(void)
{
    struct timespec now, diff;

    int retval = clock_gettime(CLOCK_MONOTONIC, &now);
    assert(retval == 0);

    timespecDiff(&lastBootAt, &now, &diff);

    return diff.tv_sec * 1000 + diff.tv_nsec / 1000000;
}
