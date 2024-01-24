#pragma once

#include <cstdint>
#include <cstdlib>

namespace teller::hal::uart {

typedef enum {
    TELEMETRY,
    DEBUG,
    NUM_UARTS,
} uart_t;

/**
 * @brief Initialization function for the UART subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

bool write(uart_t index, std::uint8_t* data, std::uint16_t size);
bool write(uart_t index, const char* data);

}
