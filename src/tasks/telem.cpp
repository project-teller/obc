#include "tasks/telem.h"

#include "hal/system.h"
#include "modules/supervisor.h"
#include "modules/telem.h"

using namespace teller::hal;
using namespace teller::supervisor;
using namespace teller::telem;

/**
 * @def BASE_TELEMETRY_FREQ_HZ
 * @brief Specifies the base telemetry frequency.
 */
#define BASE_TELEMETRY_FREQ_HZ 50

[[noreturn]] void teller::tasks::telemetryTask(void* arg)
{
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    uint32_t deadline;
    TaskRegistration task("telem");
    task.expect(BASE_TELEMETRY_FREQ_HZ - 2, BASE_TELEMETRY_FREQ_HZ + 1);

    while (true) {
        task.nudge();

        deadline = system::getTimeSinceBootMsec() + (1000 / BASE_TELEMETRY_FREQ_HZ);
        runSingleIteration(payload);

        system::sleepUntilMsec(deadline);
    }
}
