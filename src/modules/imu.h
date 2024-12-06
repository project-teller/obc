#pragma once

#include "core/telem/generic.h"

namespace teller::imu {

/**
 * @brief Initializes the state manager of the IMU subsystem.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the state manager of the IMU subsystem.
 */
void destroy(void);

/**
 * @brief Returns the most recent acceleration measurement from the IMU.
 */
teller::telem::measurement_3d_t getAcceleration(void);

/**
 * @brief Returns the most recent angular velocity measurement from the IMU.
 */
teller::telem::measurement_3d_t getAngularVelocity(void);

/**
 * @brief Returns the status of the IMU subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Sets up the IMU before entering its main loop.
 *
 * @param  logErrors  whether errors should be logged
 * @return Whether the IMU subsystem is healthy
 */
bool setup(bool logErrors);

/**
 * @brief Starts the calibration of the gyroscope.
 */
void startGyroCalibration(void);

/**
 * @brief Updates the IMU measurements.
 *
 * Blocks until a new measurement was received.
 *
 * @return Whether the IMU subsystem is healthy
 */
bool update(void);

/**
 * @brief Saves the most recent IMU measurement to the log.
 */
void log(void);

}
