#include "hal/gpio.h"
#include "hal/system.h"

#include "modules/rxsm.h"
#include "tasks/pins.h"

using namespace teller::hal;

[[noreturn]] void teller::tasks::pinsTask(void* arg)
{
    while (true) {
        teller::rxsm::update(
            gpio::read(gpio::SODS),
            gpio::read(gpio::SOE),
            gpio::read(gpio::LO));
        system::delayMsec(20);
    }
}
