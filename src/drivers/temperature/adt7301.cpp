#include "config.h"
#include "drivers/temperature.h"
#include "hal/spi.h"

using namespace teller::hal;

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
static const spi::address_t address = spi::NO_ADDRESS;
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board: SPI bus 3, CS pin 4
static const spi::address_t address = { .bus = 2, .device = 3 };
#else
// No temperature sensor on this board
static const spi::address_t address = spi::NO_ADDRESS;
#endif

namespace teller::drivers::temperature {

bool init()
{
    return true;
}

void destroy()
{
}

bool setup(void)
{
    return true;
}

bool update(float& temperature)
{
    uint8_t txBuf[2] = { 0, 0 };
    uint8_t rxBuf[2] = { 0, 0 };

    if (!spi::transfer(address, txBuf, rxBuf, sizeof(txBuf))) {
        return false;
    }

    uint16_t encodedTemperature = (rxBuf[0] << 8) | rxBuf[1];
    if (encodedTemperature & 0xc000) {
        /* invalid reading */
        return false;
    }

    if (encodedTemperature & 0x2000) {
        /* negative temperature */
        temperature = (encodedTemperature - 16384) / 32.0f;
    } else {
        /* non-negative temperature */
        temperature = encodedTemperature / 32.0f;
    }

    return true;
}

}
