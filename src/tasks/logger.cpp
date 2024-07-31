#include "hal/system.h"

#include "modules/log.h"
#include "modules/supervisor.h"
#include "tasks/logger.h"

using namespace teller::hal;
using namespace teller::supervisor;

/**
 * @def BASE_LOG_FREQ_HZ
 * @brief Specifies the base logging frequency.
 */
#define BASE_LOG_FREQ_HZ 50

[[noreturn]] void teller::tasks::loggerTask(void* arg)
{
    TaskRegistration task("log");
    task.expect(BASE_LOG_FREQ_HZ - 2, BASE_LOG_FREQ_HZ + 1);

    while (true) {
        teller::log::runSingleIteration();
        system::delayMsec(1000 / BASE_LOG_FREQ_HZ);
        task.nudge();
    }
}
