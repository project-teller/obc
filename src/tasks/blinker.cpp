#include "tasks/blinker.h"
#include "hal/led.h"
#include "hal/system.h"
#include "modules/debug.h"

using namespace teller::hal;

[[noreturn]] void teller::tasks::blinkTask(void* arg)
{
    uint8_t mask = 0b10000000;
    for (;;) {
        led::set(led::HEARTBEAT, teller::debug::getBlinkPattern() & mask);
        mask >>= 1;
        if (!mask) {
            mask = 0b10000000;
        }
        system::delayMsec(125);
    }
}
