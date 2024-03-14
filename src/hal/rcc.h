#pragma once

#include <cstdint>

namespace teller::hal::rcc {

typedef enum {
    RESET_REASON_UNKNOWN,
    RESET_REASON_NORMAL,
    RESET_REASON_SOFTWARE,
    RESET_REASON_WATCHDOG,
} reset_reason_t;

/**
 * @brief Initialization function for the RCC subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the RCC subsystem.
 *
 * This function is called from tests to reset the RCC subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Returns the reason of the last system reset.
 */
reset_reason_t getReasonOfLastReset(void);

/**
 * @brief Requests a system reset.
 */
void requestReset(void);

}
