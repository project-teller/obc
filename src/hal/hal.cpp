#include "hal.h"

#include "led.h"
#include "system.h"
#include "uart.h"

using namespace teller::hal;

bool teller::hal::init()
{
    bool success = true;

    system::init();
    led::init();

    success &= uart::init();

    return success;
}

void teller::hal::notifyFatalError()
{
    led::set(led::ERROR, true);
}
