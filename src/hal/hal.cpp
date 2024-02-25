#include "hal.h"

#include "led.h"
#include "rcc.h"
#include "rtc.h"
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
    success &= uart::init();

    return success;
}

void teller::hal::notifyFatalError()
{
    led::set(led::ERROR, true);
}
