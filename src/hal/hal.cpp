#include "hal.h"

#include "board.h"
#include "dma.h"
#include "gpio.h"
#include "led.h"
#include "rtc.h"
#include "spi.h"
#include "storage.h"
#include "system.h"
#include "uart.h"
#include "usb.h"
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
    success &= usb::init();

    /* Storage devices */
    success &= storage::init();

    return success;
}

void teller::hal::destroy()
{
    /* Reverse order compared to ::init() */

    /* Storage devices */
    storage::destroy();

    /* I/O devices and buses */
    usb::destroy();
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
