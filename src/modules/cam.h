#pragma once

#include <cstdint>

#include "core/telem/generic.h"

namespace teller::cam {

/**
 * @brief Initializes the module handling the camera.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the module handling the camera.
 */
void destroy(void);

/**
 * @brief Returns the status of the camera (ADS) subsystem.
 */
teller::telem::subsystem_status_t getSubsystemStatus(void);

/**
 * @brief Returns whether the camera is enabled.
 */
bool isEnabled(void);

/**
 * @brief Forcibly sends a pulse to toggle the camera.
 */
void sendPulse(void);

/**
 * @brief Sets whether the camera is enabled.
 */
void setEnabled(bool value);

/**
 * @brief Sets the duration of the pulse used to toggle the camera status.
 *
 * Should be used only in unit tests.
 */
void setPulseDurationMsec(uint16_t duration_msec);

}
