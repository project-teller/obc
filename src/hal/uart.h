#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

namespace teller::hal::uart {

typedef enum {
    TELEMETRY,
    DEBUG,
    NUM_UARTS,
} uart_t;

bool init(void);
bool write(uart_t index, std::uint8_t* data, std::uint16_t size);
bool write(uart_t index, const char* data);
bool write(uart_t index, const std::string& data);

}
