#pragma once

namespace teller::cmd {

/**
 * Initializes the data structures required by the command handler module.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * Destroys the data structures required by the command handler module.
 */
void destroy(void);

/**
 * @brief Handles a single command from the appropriate UART.
 *
 * Blocks indefinitely if there are no incoming commands.
 *
 * @return Whether an incoming packet was decoded and handled successfully.
 */
bool handleNext(void);

}
