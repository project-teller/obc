#pragma once

namespace teller::hal::sdcard {

/**
 * @brief Initialization function for the SD card reader.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the SD card reader.
 *
 * This function is called from tests to reset the LED subsystem to a known
 * base state.
 */
void destroy(void);

}
