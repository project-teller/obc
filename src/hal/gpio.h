#pragma once

#include <cstdint>

namespace teller::hal::gpio {

/**
 * @brief Enum containing symbolic constants for the pins in the GPIO abstraction layer.
 */
typedef enum {
    /* RXSM signals */
    SODS,
    SOE,
    LO,

    /* Latching current limiter status */
    STATUS_GMM_LCL,
    STATUS_SCM_LCL,
    STATUS_SUC_LCL1,
    STATUS_SUC_LCL2,
    STATUS_SUC_LCL3,
    STATUS_HVPSU_LCL,

    /* Latching current limiter reset */
    RST_GMM_LCL,
    RST_SCM_LCL,
    RST_SUC_LCL1,
    RST_SUC_LCL2,
    RST_SUC_LCL3,
    RST_HVPSU_LCL,

    GPIO_COUNT,
} pin_t;

/**
 * @brief Initialization function for the GPIO subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the GPIO subsystem.
 *
 * This function is called from tests to reset the GPIO subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Reads the current value of a GPIO pin.
 *
 * @param index  index of the GPIO pin to read
 * @return the current value of the GPIO pin
 */
bool read(pin_t index);

/**
 * @brief Reads the current value of a GPIO pin.
 *
 * @param index  index of the GPIO pin to read.
 * @param value  the new value to write
 */
void write(pin_t index, bool value);

}
