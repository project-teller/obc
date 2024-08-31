#pragma once

#include "core/telem/generic.h"

namespace teller::gmm {

/**
 * @brief Initializes the state manager of the GMM subsystem.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the state manager of the GMM subsystem.
 */
void destroy(void);

/**
 * @brief Returns the status of the GMM subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Sets up the GMM subsystem before entering its main loop.
 * @return whether the GMM subsystem is healthy
 */
bool setup(void);

/**
 * @brief Updates the GMM measurements.
 *
 * @param payload  buffer in which the GMM telemetry message can be assembled
 * @param updated  returns whether a new measurement was received
 *
 * @return whether the GMM subsystem is healthy
 */
bool update(uint8_t* payload, bool& updated);

}
