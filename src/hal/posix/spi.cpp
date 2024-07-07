#include "hal/spi.h"

namespace teller::hal::spi {

bool init()
{
    return true;
}

void destroy()
{
}

bool transfer(address_t address, std::uint8_t* buf, std::uint16_t size)
{
    return transfer(address, buf, buf, size);
}

bool transfer(address_t address, std::uint8_t* txBuf, std::uint8_t* rxBuf, std::uint16_t size)
{
    return false;
}

}
