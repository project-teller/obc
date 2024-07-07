#pragma once

#include <cstdint>

namespace teller::hal::imu {

typedef struct {
    uint32_t timestampInMsec;
    float x;
    float y;
    float z;
} measurement_t;

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
 * @brief Retrieves the most recent acceleration measurement.
 *
 * @param result  the measurement will be returned here
 * @return whether a new, valid measurement was returned
 */
bool getAcceleration(measurement_t& result);

/**
 * @brief Returns the most recent angular velocity measurement.
 *
 * @param result  the measurement will be returned here
 * @return whether a new, valid measurement was returned
 */
bool getAngularVelocity(measurement_t& result);

/**
 * @brief Updates the IMU measurements.
 *
 * This function should block until the next measurement becomes available.
 *
 * @return whether the IMU is healthy
 */
bool update(void);

}
