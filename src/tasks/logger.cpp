#include "hal/system.h"

#include "modules/log.h"
#include "tasks/logger.h"

using namespace teller::hal;

/**
 * @def BASE_LOG_FREQ_HZ
 * @brief Specifies the base logging frequency.
 */
#define BASE_LOG_FREQ_HZ 50

[[noreturn]] void teller::tasks::loggerTask(void* arg)
{
    while (true) {
        teller::log::runSingleIteration();
        system::delayMsec(1000 / BASE_LOG_FREQ_HZ);
    }
}
