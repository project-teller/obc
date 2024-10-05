#include "config.h"
#include "drivers/adc.h"
#include "hal/spi.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

#if defined(TELLER_BOARD_NUCLEO144)
static const spi::address_t address = spi::NO_ADDRESS;
#elif defined(TELLER_BOARD_STM32F4)
/* SPI bus 3, CS pin 5 */
static const spi::address_t address = { .bus = 2, .device = 4 };
#else
static const spi::address_t address = spi::NO_ADDRESS;
#endif

static Logger* logger;

namespace teller::drivers::adc {

bool init()
{
    /* Most of the initialization is done in setup() because we need to run
     * SPI transfers with interrupts */
    logger = getLogger(MODULE_ID_OBC);
    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
}

bool setup(void)
{
    return false;
}

bool update(std::uint8_t channel, float& value)
{
    value = 0.0f;
    return true;
}

}
