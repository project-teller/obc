#pragma once

#include "drivers/common.h"
#include <littlefs-cpp.h>

namespace teller::drivers::sdcard {

/**
 * @brief Initialization function for the SD card reader.
 *
 * This function is called from the global initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the SD card reader.
 *
 * This function is called from tests to reset the SD card reader to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Prepares the SD card.
 *
 * This function is called at startup from the EDR module before we start using
 * the SD card for logging.
 *
 * The ownership of the newly created configuration object is retained by the
 * flash memory driver.
 *
 * @return a new LittleFS filesystem configuration object, or a null pointer if
 * the SD card cannot be initialized.
 */
littlefs::FilesystemConfig* setup(void);

/**
 * @brief Returns the current operation being performed by the SD card.
 */
StorageOperation getCurrentOperation(void);

/**
 * @brief Returns the statistics of the flash memory.
 */
StorageStatistics getStatistics(void);

/**
 * @brief Returns the total size of the SD card, in bytes.
 */
uint32_t getTotalSize(void);

/**
 * @brief Reads a given number of bytes from the SD card.
 *
 * @param buf     the buffer to read into
 * @param address the address to read from
 * @param length  the numebr of bytes to read
 */
bool readData(uint8_t* buf, uint32_t address, size_t length);

}
