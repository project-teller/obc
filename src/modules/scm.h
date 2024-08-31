#pragma once

#include "core/telem/generic.h"

namespace teller::scm {

/**
 * @brief Initializes the state manager of the SCM subsystem.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the state manager of the SCM subsystem.
 */
void destroy(void);

/**
 * @brief Returns the status of the SCM subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Sets up the SCM subsystem before entering its main loop.
 * @return whether the SCM subsystem is healthy
 */
bool setup(void);

/**
 * @brief Updates the SCM measurements.
 *
 * @param payload  buffer in which the SCM telemetry message can be assembled
 * @param updated  returns whether a new measurement was received
 *
 * @return whether the SCM subsystem is healthy
 */
bool update(uint8_t* payload, bool& updated);

}
