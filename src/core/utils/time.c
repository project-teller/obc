#include "core/utils/time.h"

void timespecDiff(
    const struct timespec* start, const struct timespec* stop,
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

int64_t timespecToMsec(const struct timespec* spec)
{
    return spec->tv_sec * 1000 + spec->tv_nsec / 1000000;
}
