#pragma once

#include "hal/system.h"

namespace teller::storage {

/**
 * @brief Removes all files that the storage subsystem might have created.
 *
 * Must be called only when the HAL storage is not initialized.
 */
void removeAllFiles(void);

}
