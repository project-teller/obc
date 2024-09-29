#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>

#include <sys/select.h>
#include <unistd.h>

#include "hal/system.h"
#include "hal/uart.h"
#include "lib/socketstream/socketstream.hh"
#include "uart_debug.h"

using swoope::socketstream;

using namespace std;
using namespace teller::hal::uart;

/** Null buffer that can be used to create a null output stream */
class NullBuffer : public streambuf {
public:
    int overflow(int c) { return c; }
};

static NullBuffer nullBuffer;
static iostream nullStream(&nullBuffer);

/** Service name or port for the debug port */
static string debugServerPort;

/** TCP socket server that is used to simulate the debug port */
static std::unique_ptr<socketstream> debugServerSocket;

/** TCP socket client that is non-null when a client is connected to the debug port */
static std::unique_ptr<socketstream> debugClientSocket;

/** File descriptor on which we can read the measurements from the GMM */
static int gmmFileDescriptor = 0;

/** File descriptor on which we can read the measurements from the SCM */
static int scmFileDescriptor = 0;

static void handleDebugPort(void);
static int uartToInputFileDescriptor(uart_t index);
static istream& uartToInputStream(uart_t index);
static ostream& uartToOutputStream(uart_t index);

static map<uart_t, stringstream> uartInputOverrides;
static map<uart_t, stringstream> uartOutputOverrides;

#define NEVER_READABLE_FILE_DESCRIPTOR -2
#define ALWAYS_READABLE_FILE_DESCRIPTOR -1

namespace teller::hal::uart {

const uint32_t WAIT_FOREVER = std::numeric_limits<uint32_t>::max();

void setDebugPort(const std::string& service);
void setGMMFileDescriptor(int fd);
void setSCMFileDescriptor(int fd);

}

bool teller::hal::uart::init()
{
    // This won't work on Windows yet as Windows is doing the dreaded
    // CRLF translation behind the scenes.
    cout.setf(ios::unitbuf);
    return true;
}

void teller::hal::uart::destroy()
{
    teller::hal::uart::setDebugPort("");
}

bool teller::hal::uart::isConnected(uart_t index)
{
    if (index != DEBUG) {
        return true;
    } else {
        /* Return true if a client is connected to the TCP socket simulating
         * the debug UART, false otherwise */
        return debugClientSocket.get() != nullptr;
    }
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
    bool result = false;

    if (stream.rdstate() & (stream.failbit | stream.eofbit)) {
        if (bytes_read) {
            *bytes_read = 0;
        }
    } else {
        stream.read(reinterpret_cast<char*>(data), size);

        if (bytes_read) {
            *bytes_read = stream.gcount();
        }

        result = stream.gcount() > 0 || !(stream.rdstate() & (stream.failbit | stream.eofbit));
    }

    if (!result) {
        // Read error. If this was the debug socket, close it. TODO(ntamas)
    }

    return result;
}

void teller::hal::uart::setDebugPort(const std::string& service)
{
    // Close the connected debug client, if any
    if (debugClientSocket) {
        debugClientSocket->shutdown(ios_base::out);
        debugClientSocket->close();
        debugClientSocket.reset(nullptr);
    }

    // Close the debug socket
    if (debugServerSocket) {
        debugServerSocket->close();
        debugServerSocket.reset(nullptr);
    }

    debugServerPort = service;

    if (!debugServerPort.empty()) {
        // Open the debug socket
        debugServerSocket = make_unique<socketstream>();
        debugServerSocket->open(debugServerPort, 4);
    }
}

void teller::hal::uart::setGMMFileDescriptor(int fd)
{
    gmmFileDescriptor = fd;
}

void teller::hal::uart::setSCMFileDescriptor(int fd)
{
    scmFileDescriptor = fd;
}

void teller::hal::uart::waitUntilConnected(uart_t index)
{
    while (!isConnected(index)) {
        if (index == DEBUG && !debugServerPort.empty()) {
            socketstream client;

            debugServerSocket->accept(client);
            if (client.is_open()) {
                debugClientSocket.reset(new socketstream());
                debugClientSocket->swap(client);
                (*debugClientSocket) << "Hello world\n";
            }
        } else {
            teller::hal::system::sleepForever();
        }
    }
}

void teller::hal::uart::waitUntilDisconnected(uart_t index)
{
    while (isConnected(index)) {
        if (index == DEBUG) {
            while (debugClientSocket.get() != nullptr) {
                teller::hal::system::delayMsec(200);
            }
        } else {
            teller::hal::system::sleepForever();
        }
    }
}

bool teller::hal::uart::write(uart_t index, uint8_t* data, uint16_t size)
{
    ostream& stream = uartToOutputStream(index);
    bool result = false;

    stream.write(reinterpret_cast<const char*>(data), size);
    if (stream.bad()) {
        if (index == DEBUG) {
            // Close the client socket
            debugClientSocket->close();
            debugClientSocket.reset(nullptr);
        }
        return false;
    }

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
    case DEBUG:
        return debugClientSocket ? *debugClientSocket : nullStream;
    default:
        return nullStream;
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
        return debugClientSocket ? *debugClientSocket : nullStream;
    default:
        return nullStream;
    }
}

static int uartToInputFileDescriptor(uart_t index)
{
    auto it = uartInputOverrides.find(index);
    if (it != uartInputOverrides.end()) {
        // UART input is overridden. We just assume that it is always readable.
        return ALWAYS_READABLE_FILE_DESCRIPTOR;
    }

    switch (index) {
    case TELEMETRY:
        return STDIN_FILENO;
    case GMM:
        return gmmFileDescriptor > 0 ? gmmFileDescriptor : NEVER_READABLE_FILE_DESCRIPTOR;
    case SCM:
        return scmFileDescriptor > 0 ? scmFileDescriptor : NEVER_READABLE_FILE_DESCRIPTOR;
    case DEBUG:
        return debugClientSocket ? debugClientSocket->rdbuf()->socket() : NEVER_READABLE_FILE_DESCRIPTOR;
    default:
        return NEVER_READABLE_FILE_DESCRIPTOR;
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

void UARTOutputRedirector::clear()
{
    auto it = uartOutputOverrides.find(_index);
    if (it != uartOutputOverrides.end()) {
        it->second.str("");
    }
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
    clear();
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
