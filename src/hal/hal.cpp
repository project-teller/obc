#include "hal.h"

#include "board.h"
#include "gpio.h"
#include "imu.h"
#include "led.h"
#include "rtc.h"
#include "spi.h"
#include "storage.h"
#include "system.h"
#include "uart.h"
#include "watchdog.h"

using namespace teller::hal;

bool teller::hal::init()
{
    bool success = true;

    system::init();
    led::init();

    success &= board::init();
    success &= watchdog::init();
    success &= rtc::init();
    success &= gpio::init();
    success &= spi::init();
    success &= uart::init();
    success &= storage::init();
    success &= imu::init(); /* depends on SPI */

    return success;
}

void teller::hal::destroy()
{
    /* Reverse order compared to ::init() */
    storage::destroy();
    uart::destroy();
    spi::destroy();
    gpio::destroy();
    rtc::destroy();
    board::destroy();
    watchdog::destroy();
    board::destroy();

    led::destroy();
    system::destroy();
}

void teller::hal::notifyFatalError()
{
    led::set(led::ERROR, true);
}
