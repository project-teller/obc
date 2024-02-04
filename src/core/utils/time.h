#pragma once

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

#ifdef __cplusplus
}
#endif
