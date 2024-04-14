#pragma once

namespace teller::edr {

/**
 * Initializes the data structures required by the experiment data recorder module.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * Destroys the data structures required by the experiment data recorder module.
 */
void destroy(void);

}
