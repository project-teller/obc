#pragma once

namespace teller::hal::usb {

/**
 * @brief Initialization function for the USB subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the USB subsystem.
 *
 * This function is called from tests to reset the USB subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Prepares the USB subsystem.
 *
 * This is where the real initialization will happen, after we have started the
 * FreeRTOS scheduler.
 */
[[nodiscard]] bool setup(void);

}
