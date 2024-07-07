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
uint32_t getTimeSinceBootMsec(void);

/**
 * @brief Delays the execution of the current thread or task with the given
 * number of milliseconds.
 */
void delayMsec(uint32_t delay);

/**
 * @brief Requests a system reset.
 */
void requestReset(void);

/**
 * @brief Suspends the execution of the current thread or task forever.
 */
[[noreturn]] void sleepForever(void);

}
