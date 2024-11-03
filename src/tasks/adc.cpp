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
    TaskRegistration task("adc");
    task.expect(BASE_ADC_UPDATE_FREQ_HZ - 2, BASE_ADC_UPDATE_FREQ_HZ + 1);

    while (true) {
        healthy = adc::setup();
        while (healthy) {
            healthy = adc::update();
            task.nudge();

            hal::system::delayMsec(1000 / BASE_ADC_UPDATE_FREQ_HZ);
        }

        hal::system::delayMsec(1000);
    }
}
