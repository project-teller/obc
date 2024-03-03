#include "core/rxsm.h"
#include "hal/gpio.h"
#include "hal/led.h"
#include "hal/system.h"

#include "tasks/pins.h"

using namespace teller::hal;

static teller::rxsm::StateManager rxsmStateManager;

[[noreturn]] void teller::tasks::pinsTask(void* arg)
{
    uint8_t values, iter, index, diff;
    teller::rxsm::State states[2];

    for (iter = 0;; iter++) {
        values = 0;

        if (gpio::read(gpio::SODS)) {
            values |= teller::rxsm::signal::SODS;
        }
        if (gpio::read(gpio::SOE)) {
            values |= teller::rxsm::signal::SOE;
        }
        if (gpio::read(gpio::LO)) {
            values |= teller::rxsm::signal::LO;
        }

        index = iter & 1;

        rxsmStateManager.update(values);
        rxsmStateManager.getState(states[index]);

        // diff = states[index].compare(states[1 - index]);
        diff = 0;
        if (diff) {
            /* State changed, inject messages */
        }

        if (iter & 63 == 0) {
        }

        system::delayMsec(20);
    }
}
