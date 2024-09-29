#include "hal/usb.h"

namespace teller::hal::usb {

bool init()
{
    return true;
}

void destroy()
{
}

/* TODO: move the handling of the USB debug port via sockets here */
bool isConnected()
{
    return false;
}

bool read(std::uint8_t* data, std::uint16_t size, std::uint16_t* bytes_read)
{
    return false;
}

bool write(std::uint8_t* data, std::uint16_t size)
{
    return false;
}

}
