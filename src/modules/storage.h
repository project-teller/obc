#pragma once

#include "core/telem/generic.h"
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
[[nodiscard]] bool init(InitMode mode = INIT_MODE_DEFAULT);

/**
 * @brief Destroys the storage subsystem.
 */
void destroy(void);

/**
 * @brief Returns the status of the storage subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Erases a storage area in the storage subsystem.
 * @return POSIX error code; zero if the operation was successful
 */
[[nodiscard]] int eraseStorage(teller::telem::storage_area_t area);

/**
 * @brief Returns whether a storage area is mounted in the storage subsystem.
 */
bool isStorageMounted(teller::telem::storage_area_t area);

/**
 * @brief Marks a storage area as having an error.
 */
void markStorageAsErrored(teller::telem::storage_area_t area);

/**
 * @brief Mounts a storage area in the storage subsystem.
 * @return POSIX error code; zero if the operation was successful
 * or if the storage area was already mounted.
 */
[[nodiscard]] int mountStorage(teller::telem::storage_area_t area);

/**
 * @brief Unmounts a storage area in the storage subsystem.
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

}
