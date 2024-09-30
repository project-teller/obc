#include "hal/usb.h"

namespace teller::hal::usb {

bool init()
{
    return true;
}

void destroy()
{
}

bool setup()
{
    return true;
}

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
