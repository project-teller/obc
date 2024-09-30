#pragma once

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
 */
bool setup(void);

/**
 * @brief Creates a LittleFS filesystem configuration object for the SD card.
 *
 * The ownership of the newly created configuration object is passed to the
 * caller.
 *
 * @return a new LittleFS filesystem configuration object, or a null pointer if
 * the SD card is not initialized yet.
 */
std::unique_ptr<littlefs::FilesystemConfig> createFilesystemConfiguration(void);

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
