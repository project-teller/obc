#pragma once

#include <cstdint>

namespace teller::hal::system {

typedef enum {
    RESET_REASON_UNKNOWN,
    RESET_REASON_NORMAL,
    RESET_REASON_SOFTWARE,
    RESET_REASON_WATCHDOG,
} reset_reason_t;

/**
 * @brief System initialization function
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
void init(void);

/**
 * @brief System destructor function
 *
 * This function is called from tests to reset the system to a known base state.
 */
void destroy(void);

/**
 * @brief Returns the reason of the last system reset.
 */
reset_reason_t getReasonOfLastReset(void);

/**
 * @brief Returns the time elapsed since boot, in milliseconds.
 */
std::uint32_t getTimeSinceBootMsec(void);

}
