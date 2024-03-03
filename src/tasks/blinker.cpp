#include "tasks/blinker.h"
#include "hal/led.h"
#include "hal/system.h"

using namespace teller::hal;

[[noreturn]] void teller::tasks::blinkTask(void* arg)
{
    for (;;) {
        led::set(led::HEARTBEAT);
        system::delayMsec(100);
        led::clear(led::HEARTBEAT);
        system::delayMsec(100);
        led::set(led::HEARTBEAT);
        system::delayMsec(100);
        led::clear(led::HEARTBEAT);
        system::delayMsec(700);
    }
}
