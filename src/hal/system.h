#pragma once

#include <cstdint>

namespace teller::hal::system {

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
 * @brief Returns the time elapsed since boot, in milliseconds.
 */
std::uint32_t getTimeSinceBootMsec(void);

}
