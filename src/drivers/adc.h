#include <cstdint>

namespace teller::drivers::adc {

/**
 * @brief Initialization function for the analog-digital converter.
 *
 * This function is called from the global initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the analog-digital converter.
 *
 * This function is called from tests to reset the sensor to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Prepares the analog-digital converter.
 *
 * This function is called at startup from the sensor reading task before we
 * start using the sensor.
 */
bool setup(void);

/**
 * @brief Retrieves a new measurement from the analog-digital converter.
 *
 * @param index  the index of the channel to read
 * @param value  the measurement will be stored here
 * @return whether the retrieval was successful
 */
bool update(std::uint8_t index, float& value);

}
