#pragma once

#include <cstdint>

namespace teller::hal::gpio {

/**
 * @brief Enum containing symbolic constants for the digital pins in the GPIO abstraction layer.
 */
typedef enum {
    DGPIO_SODS,
    DGPIO_SOE,
    DGPIO_LO,
    DGPIO_COUNT,
} digital_pin_t;

/**
 * @brief Enum containing symbolic constants for the analog pins in the GPIO abstraction layer.
 */
typedef enum {
    AGPIO_BOARD_VOLTAGE,
    AGPIO_COUNT,
} analog_pin_t;

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
 * @brief Reads the current value of an analog GPIO pin.
 *
 * @param index  index of the GPIO pin to read
 * @return the current value of the GPIO pin
 */
uint16_t readAnalog(analog_pin_t index);

/**
 * @brief Reads the current value of a digital GPIO pin.
 *
 * @param index  index of the GPIO pin to read
 * @return the current value of the GPIO pin
 */
bool readDigital(digital_pin_t index);

/**
 * @brief Writes a new value to an analog GPIO pin.
 *
 * @param index  index of the GPIO pin to write
 * @param value  the new value to write
 */
void writeAnalog(analog_pin_t index, uint16_t value);

/**
 * @brief Reads the current value of a digital GPIO pin.
 *
 * @param index  index of the GPIO pin to read.
 * @param value  the new value to write
 */
void writeDigital(digital_pin_t index, bool value);

}
