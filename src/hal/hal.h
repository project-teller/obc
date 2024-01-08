#pragma once

namespace teller::hal {

/**
 * @brief Initialization function for the hardware abstraction layer.
 *
 * This function must be called by the main function at startup time. The
 * function initializes all subsystems and returns whether the initialization
 * was successful.
 *
 * @return whether the initialization was successful for all subsystems.
 */
bool init(void);

/**
 * @brief Notifies the hardware abstraction layer about fatal errors.
 *
 * This function must be called when the underlying RTOS detects a fatal error
 * such as a stack overflow or a failed memory allocation.
 */
void notifyFatalError(void);

}
