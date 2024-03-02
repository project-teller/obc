#pragma once

#include <cstdint>

namespace teller::hal::gpio {

/**
 * @brief Enum containing symbolic constants for the pins in the GPIO abstraction layer.
 */
typedef enum {
    GPIO_SODS,
    GPIO_SOE,
    GPIO_LO,
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
