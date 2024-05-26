#include "hal.h"

#include "gpio.h"
#include "imu.h"
#include "led.h"
#include "rcc.h"
#include "rtc.h"
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

    success &= rcc::init();
    success &= watchdog::init();
    success &= rtc::init();
    success &= gpio::init();
    success &= uart::init();
    success &= storage::init();
    success &= imu::init();

    return success;
}

void teller::hal::destroy()
{
    /* Reverse order compared to ::init() */
    storage::destroy();
    uart::destroy();
    gpio::destroy();
    rtc::destroy();
    watchdog::destroy();
    rcc::destroy();

    led::destroy();
    system::destroy();
}

void teller::hal::notifyFatalError()
{
    led::set(led::ERROR, true);
}
