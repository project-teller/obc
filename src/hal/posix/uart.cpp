#include <cstring>
#include <iostream>

#include "hal/uart.h"

using namespace teller::hal::uart;

/** Null buffer that can be used to create a null output stream */
class NullBuffer : public std::streambuf {
public:
    int overflow(int c) { return c; }
};

static NullBuffer null_buffer;
std::ostream null_stream(&null_buffer);

static std::ostream& uartToStream(uart_t index);

bool teller::hal::uart::init()
{
    std::cout.setf(std::ios::unitbuf | std::ios::binary);
    return true;
}

bool teller::hal::uart::write(uart_t index, uint8_t* data, uint16_t size)
{
    uartToStream(index).write(reinterpret_cast<const char*>(data), size);
    return true;
}

bool teller::hal::uart::write(uart_t index, const char* data)
{
    return write(index, reinterpret_cast<uint8_t*>(const_cast<char*>(data)), strlen(data));
}

static std::ostream& uartToStream(uart_t index)
{
    switch (index) {
    case TELEMETRY:
        return std::cout;
    case DEBUG:
        return std::cerr;
    default:
        return null_stream;
    }
}
