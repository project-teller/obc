#include <time.h>

#include "config.h"
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

int64_t utcTimeToMsec(
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second,
    uint16_t millisecond)
{
    struct tm brokenTime;
    time_t timestamp;

    if (year < 1970) {
        return 0;
    }

    brokenTime.tm_year = year - 1900;
    brokenTime.tm_mon = month - 1; /* months are 0-based in mktime() */
    brokenTime.tm_mday = day;
    brokenTime.tm_hour = hour;
    brokenTime.tm_min = minute;
    brokenTime.tm_sec = second;
    brokenTime.tm_isdst = 0;

#ifdef HAVE_TIMEGM
    timestamp = timegm(&brokenTime);
#else
    timestamp = mktime(&brokenTime);
#endif
    if (timestamp < 0) {
        return 0;
    }

    return timestamp * 1000 + millisecond;
}
