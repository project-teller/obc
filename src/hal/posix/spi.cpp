#include "hal/spi.h"

namespace teller::hal::spi {

const address_t NO_ADDRESS = { 0xFF, 0xFF };
const transfer_t NO_MORE_TRANSFERS = { 0, 0, 0 };

bool init()
{
    return true;
}

void destroy()
{
}

bool select(address_t address, bool value)
{
    return false;
}

bool setClockSpeed(std::uint8_t bus, std::uint32_t speed, std::uint32_t* result)
{
    if (result) {
        *result = speed;
    }

    return true;
}

bool transfer(
    address_t address, std::uint8_t* buf, std::uint16_t size, std::uint8_t flags)
{
    return transfer(address, buf, buf, size);
}

bool transfer(
    address_t address, std::uint8_t* txBuf, std::uint8_t* rxBuf, std::uint16_t size,
    std::uint8_t flags)
{
    return false;
}

bool transfer(
    address_t address, const transfer_t* transfers, std::uint16_t count,
    std::uint8_t flags)
{
    return false;
}

bool unselect(address_t address)
{
    return false;
}

}
