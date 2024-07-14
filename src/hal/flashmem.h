#pragma once

namespace teller::hal::flashmem {

/**
 * @brief Initialization function for the flash memory.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the flash memory.
 *
 * This function is called from tests to reset the LED subsystem to a known
 * base state.
 */
void destroy(void);

}
