#include "hal.h"

#include "led.h"
#include "system.h"
#include "uart.h"

bool teller::hal::init()
{
    bool success = true;

    teller::hal::system::init();
    teller::hal::led::init();

    /*
    success = teller::hal::uart::init();

    if (!success) {
        teller::hal::led::set(teller::hal::led::ERROR);
    }
    */

    return success;
}
