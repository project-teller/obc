#pragma once

#include <cstdint>

namespace teller::hal::board {

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
 * @brief Retrieves the most recent board temperature measurement.
 *
 * @return the most recent temperature measurement
 */
float getBoardTemperature(void);

/**
 * @brief Returns the most recent board voltage measurement.
 *
 * @return the most recent board voltage measurement; 0.0 in case of an error
 */
float getBoardVoltage(void);

/**
 * @brief Returns the reason of the last system reset.
 */
reset_reason_t getReasonOfLastReset(void);

}
