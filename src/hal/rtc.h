#pragma once

#include <cstdint>

namespace teller::hal::rtc {

/**
 * @brief Initialization function for the RTC subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the RTC subsystem.
 *
 * This function is called from tests to reset the LED subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Returns the current timestamp of the RTC, in milliseconds.
 */
uint64_t getTimeMsec(void);

/**
 * @brief Sets the current timestamp of the RTC, in milliseconds.
 *
 * @param  timestamp  the new timestamp to set
 * @return whether the operation was successful
 */
[[nodiscard]] bool setTimeMsec(uint64_t timestamp);

}
