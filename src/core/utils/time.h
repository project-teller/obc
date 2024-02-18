#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculates the difference between two timespec structs.
 *
 * @param start  The first timespec struct.
 * @param stop   The second timespec struct.
 * @param result The difference between the two timespecs, positive if the first
 *        timespec is earlier.
 */
void timespecDiff(const struct timespec* start, const struct timespec* stop, struct timespec* result);

/**
 * @brief Converts a timespec struct to milliseconds.
 */
int64_t timespecToMsec(const struct timespec* spec);

/**
 * @brief Converts a UTC time instant in broken time representation to milliseconds.
 */
int64_t utcTimeToMsec(
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second,
    uint16_t millisecond);

#ifdef __cplusplus
}
#endif
