#pragma once

#include <cstdint>

namespace teller::lcl {

/**
 * @brief Enum for the latching current limiters on the hardware.
 */
typedef enum {
    GMM_LCL,
    SCM_LCL,
    SUC_LCL1,
    SUC_LCL2,
    SUC_LCL3,
    CAM_LCL,
    NUM_LCLS,
} lcl_t;

/**
 * @brief Initializes the module handling the latching current limiters.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the module handling the latching current limiters.
 */
void destroy(void);

/**
 * @brief Returns whether the given LCL was triggered.
 */
bool triggered(lcl_t lcl);

/**
 * @brief Resets the given LCL.
 */
void reset(lcl_t lcl);

/**
 * @brief Resets multiple LCLs identified by bits in a byte.
 */
void resetMultiple(uint8_t lcls_to_reset);

/**
 * @brief Sets the duration of the pulse used to reset an LCL.
 *
 * Should be used only in unit tests.
 */
void setResetPulseDurationMsec(uint16_t duration_msec);

}
