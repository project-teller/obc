#include "modules/supervisor.h"
#include "hal/system.h"
#include "tasks/supervisor.h"

using namespace teller::hal;

[[noreturn]] void teller::tasks::supervisorTask(void* arg)
{
    supervisor::setup();
    for (;;) {
        system::delayMsec(1000);
        supervisor::checkTasks();
    }
}
