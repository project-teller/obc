#include "tasks/scheduler.h"

#include "hal/system.h"
#include "modules/scheduler.h"
#include "modules/supervisor.h"

using namespace teller;
using namespace teller::scheduler;
using namespace teller::supervisor;

/**
 * @def BASE_SCHEDULER_UPDATE_FREQ_HZ
 * @brief Specifies the base update frequency that we expect from the scheduler.
 */
#define BASE_SCHEDULER_UPDATE_FREQ_HZ 10

[[noreturn]] void teller::tasks::schedulerTask(void* arg)
{
    TaskRegistration task("scheduler");
    task.expect(BASE_SCHEDULER_UPDATE_FREQ_HZ - 3, BASE_SCHEDULER_UPDATE_FREQ_HZ + 1);

    while (true) {
        scheduler::update();
        task.nudge();
        hal::system::delayMsec(100);
    }
}
