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
 * @brief Handles pending incoming command bytes from the appropriate UART.
 *
 * Blocks indefinitely if there is nothing to read from the UART.
 *
 * @return Whether an incoming packet was decoded and handled successfully.
 */
bool handleCommands(void);

}
