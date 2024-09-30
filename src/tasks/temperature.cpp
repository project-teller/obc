#include "tasks/temperature.h"
#include "drivers/temperature.h"
#include "hal/board.h"
#include "hal/system.h"
#include "modules/errors.h"

using namespace teller::drivers;
using namespace teller::errors;
using namespace teller::hal;

[[noreturn]] void teller::tasks::temperatureTask(void* arg)
{
    float temperature;

    if (!temperature::setup()) {
        system::sleepForever();
    }

    for (;;) {
        bool ok = temperature::update(temperature);

        if (ok) {
            board::updateBoardTemperature(temperature);
        }

        setError(TEMPERATURE_SENSOR_FAILED, !ok);

        system::delayMsec(1000);
    }
}
