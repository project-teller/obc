#pragma once

#include "core/telem/generic.h"

namespace teller::mag {

/**
 * @brief Initializes the state manager of the magnetometer subsystem.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the state manager of the magnetometer subsystem.
 */
void destroy(void);

/**
 * @brief Returns the most recent magnetic vector measurement from the magnetometer.
 */
teller::telem::measurement_3d_t getMagneticVector(void);

/**
 * @brief Returns the status of the magnetometer subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Sets up the magnetometer before entering its main loop.
 * @return Whether the magnetometer subsystem is healthy
 */
bool setup(void);

/**
 * @brief Updates the magnetometer measurements.
 *
 * Blocks until a new measurement was received.
 *
 * @return Whether the magnetometer is healthy
 */
bool update(void);

/**
 * @brief Saves the most recent magnetometer measurement to the log.
 */
void log(void);

}
