#include "tasks/mission.h"

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

[[noreturn]] void teller::tasks::missionTask(void* arg)
{
    TaskRegistration task("mission");
    task.expect(BASE_SCHEDULER_UPDATE_FREQ_HZ - 3, BASE_SCHEDULER_UPDATE_FREQ_HZ + 1);

    while (true) {
        /* Update the scheduler and perform scheduled events*/
        scheduler::update();

        /* Check the status of LCLs and reset them as needed _if_ we are after
         * the liftoff */

        /* Nudge the task supervisor */
        task.nudge();
        hal::system::delayMsec(100);
    }
}
