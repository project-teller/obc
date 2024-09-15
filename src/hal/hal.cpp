#include "hal.h"

#include "board.h"
#include "dma.h"
#include "flashmem.h"
#include "gpio.h"
#include "imu.h"
#include "led.h"
#include "rtc.h"
#include "sdcard.h"
#include "spi.h"
#include "storage.h"
#include "system.h"
#include "uart.h"
#include "watchdog.h"

using namespace teller::hal;

bool teller::hal::init()
{
    bool success = true;

    /* Base system */
    system::init();
    led::init();

    /* Low-level stuff */
    success &= board::init();
    success &= watchdog::init();
    success &= rtc::init();
    success &= dma::init();

    /* I/O devices and buses */
    success &= gpio::init();
    success &= spi::init();
    success &= uart::init();

    /* Storage devices */
    success &= flashmem::init(); /* depends on SPI */
    success &= sdcard::init(); /* depends on SPI */
    success &= storage::init();

    /* Sensors */
    success &= imu::init(); /* depends on SPI */

    return success;
}

void teller::hal::destroy()
{
    /* Reverse order compared to ::init() */

    /* Sensors */
    imu::destroy();

    /* Storage devices */
    storage::destroy();
    sdcard::destroy();
    flashmem::destroy();

    /* I/O devices and buses */
    uart::destroy();
    spi::destroy();
    gpio::destroy();

    /* Low-level stuff */
    dma::destroy();
    rtc::destroy();
    watchdog::destroy();
    board::destroy();

    /* Base system */
    led::destroy();
    system::destroy();
}

void teller::hal::notifyFatalError()
{
    led::set(led::ERROR, true);
}
