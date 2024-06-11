#pragma once

#include <cstdint>

#include "core/telem/generic.h"

namespace teller::mode {

/**
 * @brief Initializes the module handling whether the experiment is in mission or test mode.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the module handling the experiment mode.
 */
void destroy(void);

/**
 * @brief Returns the current experiment mode.
 */
teller::telem::obc_mode_t getMode(void);

}
