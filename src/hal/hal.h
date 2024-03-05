#pragma once

namespace teller::hal {

/**
 * @brief Initialization function for the entire hardware abstraction layer.
 *
 * This function must be called by the main function at startup time. The
 * function initializes all subsystems and returns whether the initialization
 * was successful.
 *
 * @return whether the initialization was successful for all subsystems.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the entire hardware abstraction layer.
 *
 * This function is called from tests to reset the HAL to a known base state
 * by resetting all the subsystems.
 */
void destroy(void);

/**
 * @brief Notifies the hardware abstraction layer about fatal errors.
 *
 * This function must be called when the underlying RTOS detects a fatal error
 * such as a stack overflow or a failed memory allocation.
 */
void notifyFatalError(void);

}
