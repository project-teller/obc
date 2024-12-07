#pragma once

#include "drivers/common.h"
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
 *
 * The ownership of the newly created configuration object is retained by the
 * flash memory driver.
 *
 * @return a new LittleFS filesystem configuration object, or a null pointer if
 * the flash memory cannot be initialized.
 */
littlefs::FilesystemConfig* setup(void);

/**
 * @brief Returns the current operation being performed by the flash memory.
 */
StorageOperation getCurrentOperation(void);

/**
 * @brief Returns the statistics of the flash memory.
 */
StorageStatistics getStatistics(void);

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
