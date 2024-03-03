#include "core/rxsm.h"
#include "hal/gpio.h"
#include "hal/led.h"

#include "tasks/pins.h"

using namespace teller::hal;

const osThreadAttr_t teller::tasks::pinsTaskAttr = {
    .name = "pins",
    .priority = osPriorityNormal,
};

static teller::rxsm::StateManager rxsmStateManager;

__NO_RETURN void teller::tasks::pinsTask(void* arg)
{
    uint8_t values, iter, index, diff;
    teller::rxsm::State states[2];
    teller::rxsm::State* state;

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

        osDelay(20);
    }
}
