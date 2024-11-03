#include <cstring>

#include "config.h"
#include "drivers/adc.h"
#include "hal/gpio.h"
#include "hal/spi.h"
#include "hal/system.h"
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

static const gpio::pin_t readyPin = gpio::ADC_READY;
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
    uint8_t buf[] = { 0, 0 };

    /* Default clock mode is 10, meaning that conversions are timed internally
     * and can be started via the SPI bus. This is what we need.
     *
     * In clock mode 10, the wake-up, acquisition, conversion, and shutdown
     * sequences are initiated by writing an input data byte to the conversion
     * register, and are performed automatically using the internal oscillator.
     *
     * Initiate a scan by writing a byte to the conversion register. The MAX11643
     * then powers up, scans all requested channels, stores the results in the
     * FIFO, and shuts down. After the scan is complete, EOC is pulled low and
     * the results are available in the FIFO.
     *
     * The 8-bit conversion result is output in MSB-first format with four
     * leading zeros followed by 8-bit data and four trailing zeros.
     */

    /* Reset register: 0001 /RESET XXX
     * where /RESET must be set to zero to reset all registers and 1 to clear
     * the FIFO only.
     * See Table 5 of the datasheet */
    buf[0] = 0b00010000;
    if (!spi::transfer(address, buf, 1)) {
        return false;
    }

    /* Setup register: 01 CKSEL[1:0] REFSEL[1:0] X X
     * where CKSEL[1:0] must be 10
     *   and REFSEL[1:0] must be 10 (reference always on, no wake-up delay).
     * See Table 3 of the datasheet */
    buf[0] = 0b01101000;
    if (!spi::transfer(address, buf, 1)) {
        return false;
    }

    /* Averaging register: 001 AVGON NAVG[1:0] NSCAN[1:0]
     * where AVGON is whether averaging should be on or off
     *   and NAVG[1:0] must be 00, 01, 10 or 11 for 4, 8, 16 and 32 conversions
     *   and NSCAN[1:0] is XX because we are not in sacn mode 10.
     * See Table 4 of the datasheet */
    buf[0] = 0b00000000;
    if (!spi::transfer(address, buf, 1)) {
        return false;
    }

    return true;
}

bool update(std::uint8_t count, uint16_t* value)
{
    /* We need 32 bytes as the ADC returns 16 bits per channel, with 4 padding
     * bits in the front and the back */
    uint8_t buf[32];
    uint16_t rawValue;
    uint8_t i, j;

    if (count == 0) {
        return true;
    }

    if (count > 16) {
        count = 16;
    }

    /* Conversion register: 0 CHSEL[3:0] SCAN[1:0] X
     * where CHSEL[3:0] must be set to the index of the last channel to read
     *   and  SCAN[1:0] must be set to 00 to scan the channels
     * See Table 2 of the datasheet */
    buf[0] = 0x80 | (count << 3);

    if (!spi::transfer(address, buf, 1)) {
        return false;
    }

    /* Wait for /EOC */
    memset(buf, 0, sizeof(buf));
    i = 10;
    while (!gpio::read(gpio::ADC_READY)) {
        i--;
        if (i == 0) {
            return false;
        } else {
            system::delayMsec(1);
        }
    }

    /* Now read the results */
    if (!spi::transfer(address, buf, count * 2)) {
        return false;
    }

    /* Parse the results. Check that the bits expected to be zeros are indeed
     * zeros. */
    for (i = 0, j = 0; i < count; i++, j += 2) {
        rawValue = (buf[j] << 8) | buf[j + 1];
        if (rawValue & 0xF00F) {
            return false;
        }

        value[i] = (rawValue & 0x0FF0) >> 4;
    }

    return true;
}

}
