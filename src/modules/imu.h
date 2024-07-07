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
 * @brief Returns the status of the IMU subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Sets up the IMU before entering its main loop.
 * @return Whether the IMU subsystem is healthy
 */
bool setup(void);

/**
 * @brief Updates the IMU measurements.
 *
 * Blocks until a new measurement was received.
 *
 * @return Whether the IMU subsystem is healthy
 */
bool update(void);

/**
 * @brief Saves IMU measurements to the log and sends them via telemetry.
 */
void log(void);

}
