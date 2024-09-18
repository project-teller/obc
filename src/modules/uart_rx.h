#pragma once

#include "hal/uart.h"

namespace teller::uart_rx {

/**
 * Initializes the data structures required by the UART receiver module.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * Destroys the data structures required by the UART receiver module.
 */
void destroy(void);

/**
 * @brief Handles pending incoming bytes from a single UART.
 *
 * @param index  index of the UART to read
 * @return whether an incoming packet was decoded and handled successfully.
 */
bool read(teller::hal::uart::uart_t index);

}
