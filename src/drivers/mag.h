#pragma once

#include <cstdint>

#include "core/telem/generic.h"

namespace teller::drivers::mag {

/**
 * @brief Initialization function for the magnetometer.
 *
 * This function is called from the global initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the magnetometer.
 *
 * This function is called from tests to reset the magnetometer to a known base state.
 */
void destroy(void);

/**
 * @brief Setup function for the magnetometer.
 *
 * This function is called right before we start reading magnetometer
 * measurements in a loop.
 *
 * @return whether the setup was successful
 */
bool setup(void);

/**
 * @brief Updates the magnetometer measurement.
 *
 * This function should block until the next measurement becomes available.
 *
 * @param  magneticVector   the measurement is returned here
 */
bool update(teller::telem::measurement_3d_t& magneticVector);

}
