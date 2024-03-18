#pragma once

#include "littlefs-cpp.h"

namespace teller::hal::storage {

namespace area {
    typedef enum {
        FLASH_MEMORY,
        SD_CARD,
        NUMBER_OF_AREAS
    } area_t;
}

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
littlefs::FilesystemConfig* getFilesystemConfig(area::area_t area);

}
