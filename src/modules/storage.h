#pragma once

#include "core/telem/generic.h"
#include "drivers/common.h"
#include "littlefs-cpp.h"

namespace teller::storage {

typedef enum {
    INIT_MODE_NORMAL = 0,
    INIT_MODE_FORMAT_IF_NEEDED = 1,
    INIT_MODE_FORMAT = 2
} InitMode;

#ifdef TELLER_BOARD_POSIX
#define INIT_MODE_DEFAULT INIT_MODE_FORMAT_IF_NEEDED
#else
#define INIT_MODE_DEFAULT INIT_MODE_NORMAL
#endif

/**
 * @brief Initializes the storage subsystem.
 *
 * @param mode  whether to format all storage areas before mounting them
 */
[[nodiscard]] bool init();

/**
 * @brief Destroys the storage subsystem.
 */
void destroy(void);

void setup(InitMode mode = INIT_MODE_DEFAULT);

/**
 * @brief Returns the status of the storage subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Configures a storage area in the storage subsystem.
 *
 * @param area  the area to configure
 * @param cfg   the LittleFS filesystem configuration of the area
 *
 * @return  whether the configuration attempt was successful
 */
bool configureStorage(teller::telem::storage_area_t area, littlefs::FilesystemConfig& cfg);

/**
 * @brief Erases a storage area in the storage subsystem.
 * @return POSIX error code; zero if the operation was successful
 */
[[nodiscard]] int eraseStorage(teller::telem::storage_area_t area);

/**
 * @brief Returns the total size of the storage area, in bytes.
 */
int getStorageSize(teller::telem::storage_area_t area);

/**
 * @brief Returns whether a storage area is expected to exist in the storage
 * subsystem.
 */
bool isStorageConfigured(teller::telem::storage_area_t area);

/**
 * @brief Returns whether a storage area is marked as errored in the storage
 * subsystem.
 */
bool isStorageErrored(teller::telem::storage_area_t area);

/**
 * @brief Returns whether a storage area is mounted in the storage subsystem.
 */
bool isStorageMounted(teller::telem::storage_area_t area);

/**
 * @brief Marks a storage area as having an error.
 */
void markStorageAsErrored(teller::telem::storage_area_t area, int error = 0);

/**
 * @brief Mounts a storage area in the storage subsystem.
 *
 * @param force  whether to try and mount the storage area even if it is marked
 *        as errored
 *
 * @return POSIX error code; zero if the operation was successful
 *         or if the storage area was already mounted.
 */
[[nodiscard]] int mountStorage(teller::telem::storage_area_t area, bool force = false);

/**
 * @brief Reports the status of each storage area as text messages in the telemetry streams.
 */
void reportStatus(void);

/**
 * @brief Unmounts a storage area in the storage subsystem.
 *
 * @return POSIX error code; zero if the operation was successful
 * or if the storage area was already unmounted.
 */
[[nodiscard]] int unmountStorage(teller::telem::storage_area_t area);

/**
 * @brief Blocks the calling task until the given storage area becomes mounted.
 * Blocks indefinitely if the given storage area does not exist.
 *
 * @return Pointer to the mounted filesystem object.
 */
littlefs::Filesystem* waitUntilMounted(teller::telem::storage_area_t area);

/**
 * @brief Blocks the calling task until the given storage area becomes unmounted.
 * Blocks indefinitely if the given storage area does not exist.
 *
 * @return Pointer to the unmounted filesystem object.
 */
littlefs::Filesystem* waitUntilUnmounted(teller::telem::storage_area_t area);

/**
 * @brief Helper function to convert a LittleFS error code to a corresponding
 *        POSIX error code.
 */
int convertLittleFSErrorCode(std::optional<littlefs::Error> code);

/**
 * @brief Initiates a background task that reads the list of files in a
 *        directory on the filesystem of a storage area.
 *
 * @param  area     the storage area to read
 * @param  name     the name of the directory to list
 * @param  start    the index of the first entry to return from the directory
 * @param  length   the maximum number of entries to return
 * @param  targets  mask of the telemetry channels to send the data to
 * @param  seq_no   sequence number of the message that initiated the request
 * @return POSIX error code; zero if the operation was successful
 */
int startDirectoryListing(
    teller::telem::storage_area_t area, const char* name, uint16_t start, uint16_t count,
    uint8_t targets, uint8_t seq_no = 0);

/**
 * @brief Initiates a background task that reads the contents of a file on the
 *        filesystem of a storage area.
 *
 * @param  area     the storage area to read
 * @param  path     the full path of the file to read
 * @param  offset   the offset to start reading from
 * @param  length   the number of bytes to read
 * @param  targets  mask of the telemetry channels to send the data to
 * @param  seq_no   sequence number of the message that initiated the request
 * @param  file_size  when not null, the total size of the file is returned here
 * @return POSIX error code; zero if the operation was successful
 */
int startFileDownload(
    teller::telem::storage_area_t area, const char* path, uint32_t offset, uint16_t length,
    uint8_t targets, uint8_t seq_no = 0, uint32_t* file_size = nullptr);

/**
 * @brief Initiates a background task that reads the raw contents of the storage
 *        area from a given address.
 *
 * @param  area     the storage area to read
 * @param  address  the address to read from
 * @param  length   number of bytes to read
 * @param  targets  mask of the telemetry channels to send the data to
 * @param  seq_no   sequence number of the message that initiated the request
 * @return POSIX error code; zero if the operation was successful
 */
int startRawStorageDownload(
    teller::telem::storage_area_t area, uint64_t address, uint16_t length,
    uint8_t targets, uint8_t seq_no = 0);

/**
 * @brief Runs a a background task that reads the raw contents of the storage
 * area when a request is posted to do so.
 */
[[noreturn]] void runStorageReader(void);

}
