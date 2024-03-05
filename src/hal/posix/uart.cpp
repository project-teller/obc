#include <cstring>
#include <iostream>
#include <map>

#include "hal/uart.h"
#include "uart_debug.h"

using namespace std;
using namespace teller::hal::uart;

/** Null buffer that can be used to create a null output stream */
class NullBuffer : public streambuf {
public:
    int overflow(int c) { return c; }
};

static NullBuffer null_buffer;
iostream null_stream(&null_buffer);

static istream& uartToInputStream(uart_t index);
static ostream& uartToOutputStream(uart_t index);

static map<uart_t, stringstream> uartInputOverrides;
static map<uart_t, stringstream> uartOutputOverrides;

bool teller::hal::uart::init()
{
    // This won't work on Windows yet as Windows is doing the dreaded
    // CRLF translation behind the scenes.
    cout.setf(ios::unitbuf);
    return true;
}

void teller::hal::uart::destroy()
{
}

bool teller::hal::uart::read(uart_t index, uint8_t* data, uint16_t size, uint16_t* bytes_read)
{
    if (size == 0) {
        if (bytes_read) {
            *bytes_read = 0;
        }
        return true;
    }

    istream& stream = uartToInputStream(index);
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
    return stream.gcount() > 0 || !(stream.rdstate() & (stream.failbit | stream.eofbit));
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

static istream& uartToInputStream(uart_t index)
{
    auto it = uartInputOverrides.find(index);
    if (it != uartInputOverrides.end()) {
        return it->second;
    }

    switch (index) {
    case TELEMETRY:
        return cin;
    default:
        return null_stream;
    }
}

static ostream& uartToOutputStream(uart_t index)
{
    auto it = uartOutputOverrides.find(index);
    if (it != uartOutputOverrides.end()) {
        return it->second;
    }

    switch (index) {
    case TELEMETRY:
        return cout;
    case DEBUG:
        return cerr;
    default:
        return null_stream;
    }
}

namespace teller::hal::uart {

UARTOutputRedirector::UARTOutputRedirector(uart_t index)
    : _index(index)
{
    if (uartOutputOverrides.contains(index)) {
        throw runtime_error("UART output is already overridden");
    }

    uartOutputOverrides.emplace(index, stringstream());
}

UARTOutputRedirector::~UARTOutputRedirector()
{
    uartOutputOverrides.erase(_index);
}

string UARTOutputRedirector::get() const
{
    auto it = uartOutputOverrides.find(_index);
    if (it != uartOutputOverrides.end()) {
        return it->second.str();
    } else {
        return "";
    }
}

string UARTOutputRedirector::getAndClear()
{
    string result = get();

    auto it = uartOutputOverrides.find(_index);
    if (it != uartOutputOverrides.end()) {
        it->second.clear();
    }

    return result;
}

UARTInputRedirector::UARTInputRedirector(uart_t index)
    : _index(index)
{
    if (uartInputOverrides.contains(index)) {
        throw runtime_error("UART input is already overridden");
    }

    uartInputOverrides.emplace(index, stringstream());
}

UARTInputRedirector::~UARTInputRedirector()
{
    uartInputOverrides.erase(_index);
}

void UARTInputRedirector::clear()
{
    auto it = uartInputOverrides.find(_index);
    if (it != uartInputOverrides.end()) {
        stringstream& stream = it->second;
        stream.str("");
        stream.seekg(0, std::ios::beg);
        stream.clear();
    }
}

void UARTInputRedirector::feed(const string& value)
{
    auto it = uartInputOverrides.find(_index);
    if (it != uartInputOverrides.end()) {
        stringstream& stream = it->second;
        streampos pos = stream.tellg();
        stream.seekg(0, std::ios::end);
        stream.clear();
        stream << value;
        stream.seekg(pos);
        stream.clear();
    }
}

}
