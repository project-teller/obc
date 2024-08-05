#include "modules/supervisor.h"
#include "hal/system.h"
#include "modules/errors.h"
#include "tasks/supervisor.h"

using namespace teller::hal;

[[noreturn]] void teller::tasks::supervisorTask(void* arg)
{
    supervisor::setup();
    for (;;) {
        system::delayMsec(1000);
        teller::errors::setError(teller::errors::QUEUE_FULL, !supervisor::checkQueues());
        supervisor::checkTasks();
    }
}
