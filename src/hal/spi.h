#pragma once

namespace teller::hal::spi {

/**
 * @brief Initialization function for the SPI bus subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the SPI bus subsystem.
 *
 * This function is called from tests to reset the LED subsystem to a known
 * base state.
 */
void destroy(void);

}
