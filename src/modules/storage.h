#pragma once

#include "core/telem/generic.h"

namespace teller::storage {

/**
 * @brief Initializes the storage subsystem.
 *
 * @param format  whether to format all storage areas before mounting them
 */
[[nodiscard]] bool init(bool format = false);

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
 * @brief Mounts a storage area in the storage subsystem.
 * @return POSIX error code; zero if the operation was successful
 * or if the storage area was already mounted.
 */
[[nodiscard]] int mountStorage(teller::telem::storage_area_t area);

/**
 * @brief Unounts a storage area in the storage subsystem.
 * @return POSIX error code; zero if the operation was successful
 * or if the storage area was already unmounted.
 */
[[nodiscard]] int unmountStorage(teller::telem::storage_area_t area);

}
