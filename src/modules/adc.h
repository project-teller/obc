#pragma once

#include <cstdint>

#include "core/utils/majority_voter.h"

namespace teller::adc {

/**
 * @brief Initializes the state manager of the ADC conversion module.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the state manager of the ADC conversion module.
 */
void destroy(void);

/**
 * @brief Sets up the ADC before entering the main loop of the conversion module.
 * @return whether the ADC was configured successfully
 */
bool setup(void);

/**
 * @brief Updates the state of all ADC channels at once.
 * @return Whether the update was successful
 */
bool update(void);

};
