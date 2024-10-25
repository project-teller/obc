#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struct containing a UTC timestamp broken down into individual components.
 */
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
} broken_down_time_t;

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
 *
 * @return the timestamp expressed as the number of milliseconds elapsed since
 *         the UNIX epoch, or zero if the conversion was unsuccessful
 * @return wheher the conversion was successful
 */
bool utcTimeToMsec(const broken_down_time_t* components, uint64_t* timestamp);

/**
 * @brief Converts a UTC time instant in milliseconds to broken time representation.
 *
 * Timestamps before the UNIX epoch are not supported.
 *
 * @param  timestamp  a UTC time instant expressed in the number of milliseconds
 *         elapsed since the UNIX epoch
 * @param  components the broken time representation is returned here
 * @return wheher the conversion was successful
 */
bool utcMsecToTime(uint64_t timestamp, broken_down_time_t* components);

#ifdef __cplusplus
}
#endif
