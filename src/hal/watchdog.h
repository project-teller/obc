#pragma once

namespace teller::hal::watchdog {

/**
 * @brief Initialization function for the hardware watchdog
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
void init(void);

/**
 * @brief Resets the hardware watchdog
 *
 * This function must be called periodically after the watchdog is initialized
 * to keep the system from rebooting itself.
 */
void reset(void);

}
