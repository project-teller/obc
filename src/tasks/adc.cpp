#include "tasks/adc.h"

#include "hal/system.h"
#include "modules/adc.h"
#include "modules/supervisor.h"
#include "modules/telem.h"

using namespace teller;
using namespace teller::supervisor;
using namespace teller::telem;

/**
 * @def BASE_ADC_UPDATE_FREQ_HZ
 * @brief Specifies the base update frequency that we expect from the ADC module.
 */
#define BASE_ADC_UPDATE_FREQ_HZ 10

[[noreturn]] void teller::tasks::adcTask(void* arg)
{
    bool healthy;
    uint8_t tries = 0;
    TaskRegistration task("adc");
    task.expect(BASE_ADC_UPDATE_FREQ_HZ - 2, BASE_ADC_UPDATE_FREQ_HZ + 1);

    while (true) {
        healthy = adc::setup();

        while (healthy && tries < BASE_ADC_UPDATE_FREQ_HZ * 3) {
            if (adc::update()) {
                tries = 0;
                task.nudge();
            } else {
                tries++;
            }
            hal::system::delayMsec(1000 / BASE_ADC_UPDATE_FREQ_HZ);
        }

        hal::system::delayMsec(1000);
    }
}
