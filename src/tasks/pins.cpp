#include "hal/gpio.h"
#include "hal/system.h"

#include "modules/rxsm.h"
#include "modules/supervisor.h"
#include "tasks/pins.h"

using namespace teller::hal;
using namespace teller::supervisor;

/**
 * @def UPDATE_FREQ_HZ
 * @brief Specifies the update frequency of the RXSM pin state.
 */
#define UPDATE_FREQ_HZ 50

[[noreturn]] void teller::tasks::pinsTask(void* arg)
{
    TaskRegistration task("pins");
    task.expect(UPDATE_FREQ_HZ - 2, UPDATE_FREQ_HZ + 1);

    while (true) {
        task.nudge();
        teller::rxsm::update(
            gpio::read(gpio::SODS),
            gpio::read(gpio::SOE),
            gpio::read(gpio::LO));
        system::delayMsec(1000 / UPDATE_FREQ_HZ);
    }
}
