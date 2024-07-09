#pragma once

#include <cstdint>

#include "core/telem/generic.h"

namespace teller::hal::imu {

/**
 * @brief Initialization function for the IMU.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the IMU.
 *
 * This function is called from tests to reset the LED subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Setup function for the IMU.
 *
 * This function is called right before we start reading IMU measurements in
 * a loop.
 *
 * @return whether the setup was successful
 */
bool setup(void);

/**
 * @brief Updates the IMU measurements.
 *
 * This function should block until the next measurement becomes available.
 *
 * @param  acceleration     the acceleration measurement is returned here
 * @param  angularVelocity  the angular velocity measurement is returned here
 * @return whether the IMU is healthy
 */
bool update(
    teller::telem::measurement_3d_t& acceleration,
    teller::telem::measurement_3d_t& angularVelocity);

}
