#include <cassert>
#include <ctime>

#include "hal/system.h"

using namespace std;
using namespace teller::hal::system;

struct timespec lastBootAt;

reset_reason_t reasonOfLastReset = RESET_REASON_UNKNOWN;

static void timespec_diff(
    struct timespec* start, struct timespec* stop,
    struct timespec* result);

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

    timespec_diff(&lastBootAt, &now, &diff);

    return diff.tv_sec * 1000 + diff.tv_nsec / 1000000;
}

/* ************************************************************************* */

/* GCOVR_EXCL_START */
static void timespec_diff(
    struct timespec* start, struct timespec* stop,
    struct timespec* result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}
/* GCOVR_EXCL_STOP */
