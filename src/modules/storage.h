#pragma once

#include "core/telem/generic.h"

namespace teller::storage {

/**
 * @brief Initializes the storage subsystem.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the storage subsystem.
 */
void destroy(void);

/**
 * @brief Returns the status of the storage subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

}
