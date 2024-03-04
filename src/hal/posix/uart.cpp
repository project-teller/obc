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
std::iostream null_stream(&null_buffer);

static std::istream& uartToInputStream(uart_t index);
static std::ostream& uartToOutputStream(uart_t index);

bool teller::hal::uart::init()
{
    // This won't work on Windows yet as Windows is doing the dreaded
    // CRLF translation behind the scenes.
    std::cout.setf(std::ios::unitbuf);
    return true;
}

bool teller::hal::uart::read(uart_t index, uint8_t* data, uint16_t size, uint16_t* bytes_read)
{
    if (size == 0) {
        if (bytes_read) {
            *bytes_read = 0;
        }
        return true;
    }

    std::istream& stream = uartToInputStream(index);
    if (stream.rdstate() & (stream.failbit | stream.eofbit)) {
        if (bytes_read) {
            *bytes_read = 0;
        }
        return false;
    }

    stream.read(reinterpret_cast<char*>(data), size);
    if (bytes_read) {
        *bytes_read = stream.gcount();
    }
    return !(stream.rdstate() & (stream.failbit | stream.eofbit));
}

bool teller::hal::uart::write(uart_t index, uint8_t* data, uint16_t size)
{
    uartToOutputStream(index).write(reinterpret_cast<const char*>(data), size);
    return true;
}

bool teller::hal::uart::write(uart_t index, const char* data)
{
    return write(index, reinterpret_cast<uint8_t*>(const_cast<char*>(data)), strlen(data));
}

static std::istream& uartToInputStream(uart_t index)
{
    switch (index) {
    case TELEMETRY:
        return std::cin;
    default:
        return null_stream;
    }
}

static std::ostream& uartToOutputStream(uart_t index)
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
