#pragma once

#include "hal/system.h"

namespace teller::hal::system {

/**
 * @brief Prevent the execution of the next reset request in unit tests.
 */
void preventNextReset(void);

/**
 * @brief Returns the number of reset attempts that were prevented since last boot.
 */
size_t countPreventedResetAttempts(void);

}
