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
 * @brief Returns the most recent GMM hit counts.
 */
teller::telem::measurement_gmm_t getHitCounts(void);

/**
 * @brief Returns the status of the GMM subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Sets up the GMM subsystem before entering its main loop.
 * @return Whether the GMM subsystem is healthy
 */
bool setup(void);

/**
 * @brief Updates the GMM measurements.
 *
 * Blocks until a new GMM measurement was received.
 *
 * @return Whether the GMM subsystem is healthy
 */
bool update(void);

/**
 * @brief Saves the most recent GMM measurement to the log.
 */
void log(void);

}
