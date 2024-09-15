#pragma once

#include <cstdint>

namespace teller::hal::dma {

/**
 * @brief Initialization function for the DMA controller.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the DMA controller.
 *
 * This function is called from tests to reset the DMA controller to a known
 * base state.
 */
void destroy(void);

}
