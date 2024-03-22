#pragma once

#include "core/telem/generic.h"
#include "littlefs-cpp.h"

namespace teller::hal::storage {

/**
 * @brief Initialization function for the storage subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the stoage subsystem.
 *
 * This function is called from tests to reset the storage subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Returns the LittleFS filesystem configuration of a storage area.
 */
littlefs::FilesystemConfig* getFilesystemConfig(teller::telem::storage_area_t area);

}
