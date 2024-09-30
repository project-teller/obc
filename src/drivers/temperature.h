namespace teller::drivers::temperature {

/**
 * @brief Initialization function for the temperature sensor.
 *
 * This function is called from the global initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the temperature sensor.
 *
 * This function is called from tests to reset the sensor to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Prepares the temperature sensor.
 *
 * This function is called at startup from the sensor reading task before we
 * start using the sensor.
 */
bool setup(void);

/**
 * @brief Retrieves a new measurement from the temperature sensor.
 *
 * @param temperature  the measurement will be stored here
 * @return whether the retrieval was successful
 */
bool update(float& temperature);

}
