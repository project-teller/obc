#include "tasks/telem.h"

#include "hal/system.h"
#include "modules/telem.h"

using namespace teller::hal;
using namespace teller::telem;

/**
 * @def BASE_TELEMETRY_FREQ_HZ
 * @brief Specifies the base telemetry frequency.
 */
#define BASE_TELEMETRY_FREQ_HZ 50

[[noreturn]] void teller::tasks::telemetryTask(void* arg)
{
    uint8_t payload[MAX_PAYLOAD_LENGTH];

    while (true) {
        runSingleIteration(payload);
        system::delayMsec(1000 / BASE_TELEMETRY_FREQ_HZ);
    }
}
