#pragma once

#include "littlefs-cpp.h"
#include <memory>

namespace teller::drivers::flashmem {

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
 * This function is called from tests to reset the flash memory to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Prepares the flash memory.
 *
 * This function is called at startup from the EDR module before we start using
 * the flash memory for logging.
 */
bool setup(void);

/**
 * @brief Creates a LittleFS filesystem configuration object for the flash memory.
 *
 * The ownership of the newly created configuration object is passed to the
 * caller.
 *
 * @return a new LittleFS filesystem configuration object, or a null pointer if
 * the flash memory is not initialized yet.
 */
std::unique_ptr<littlefs::FilesystemConfig> createFilesystemConfiguration(void);

/**
 * @brief Returns the total size of the flash memory, in bytes.
 */
uint32_t getTotalSize(void);

/**
 * @brief Reads a given number of bytes from the flash memory.
 *
 * @param buf     the buffer to read into
 * @param address the address to read from
 * @param length  the numebr of bytes to read
 */
bool readData(uint8_t* buf, uint32_t address, size_t length);

}
