#pragma once

namespace teller::hal::watchdog {

/**
 * @brief Initialization function for the hardware watchdog
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 *
 * This function does \em not start the watchdog yet; call the \ref configureAndStart()
 * function to do that.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the hardware watchdog
 *
 * This function is called from test cases to reset the hardware watchdog to a
 * base state.
 */
void destroy(void);

/**
 * @brief Configures and starts the watchdog.
 *
 * After this function is called, the \ref reset() function of the watchdog must
 * be called at regular intervals to prevent the system from resetting itself.
 */
void configureAndStart(void);

/**
 * @brief Resets the hardware watchdog
 *
 * This function must be called periodically after the watchdog is initialized
 * to keep the system from rebooting itself.
 */
void reset(void);

}
