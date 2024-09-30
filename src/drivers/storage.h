#pragma once

#include "core/telem/generic.h"
#include "littlefs-cpp.h"

namespace teller::drivers::storage {

/**
 * @brief Initialization function for the storage driver.
 *
 * This function is called from the global initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the storage driver.
 *
 * This function is called from tests to reset the storage driver to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Returns the LittleFS filesystem configuration of a storage area.
 */
littlefs::FilesystemConfig* getFilesystemConfig(teller::telem::storage_area_t area);

}
