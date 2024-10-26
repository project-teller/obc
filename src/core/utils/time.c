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

bool utcTimeToMsec(const broken_down_time_t* components, uint64_t* timestamp)
{
    struct tm brokenTime;
    time_t time;

    if (!components) {
        return false;
    }

    if (components->year < 1970) {
        return false;
    }

    brokenTime.tm_year = components->year - 1900;
    brokenTime.tm_mon = components->month - 1; /* months are 0-based in mktime() */
    brokenTime.tm_mday = components->day;
    brokenTime.tm_hour = components->hour;
    brokenTime.tm_min = components->minute;
    brokenTime.tm_sec = components->second;
    brokenTime.tm_isdst = 0;

#ifdef HAVE_TIMEGM
    time = timegm(&brokenTime);
#else
    time = mktime(&brokenTime);
#endif
    if (time < 0) {
        return false;
    }

    if (timestamp) {
        *timestamp = time * 1000 + components->millisecond;
    }

    return true;
}

bool utcMsecToTime(
    uint64_t timestamp,
    broken_down_time_t* components)
{
    struct tm brokenTime;
    time_t time = timestamp / 1000;

    if (!gmtime_r(&time, &brokenTime)) {
        return false;
    }

    if (components) {
        components->year = brokenTime.tm_year + 1900;
        components->month = brokenTime.tm_mon + 1;
        components->day = brokenTime.tm_mday;
        components->hour = brokenTime.tm_hour;
        components->minute = brokenTime.tm_min;
        components->second = brokenTime.tm_sec;
        components->millisecond = timestamp % 1000;
    }

    return true;
}

int utcTimeToDayOfWeek(const broken_down_time_t* components)
{
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    int y = components->year;
    int m = components->month;
    int d = components->day;

    /* Tomohiko Sakamoto's method, question 20.31 in the C FAQ list */
    y -= m < 3;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}
