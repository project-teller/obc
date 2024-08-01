#pragma once

#include "hal/uart.h"

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
 * @param index  index of the UART to read
 * @param buf  a pre-allocated buffer where the response to the command can
 *        be composed if needed
 * @param timeout  maximum number of milliseconds to wait for the next byte to
 *        read from the UART
 * @return Whether an incoming packet was decoded and handled successfully.
 */
bool handleCommands(teller::hal::uart::uart_t index, uint8_t* buf, uint32_t timeout);

}
