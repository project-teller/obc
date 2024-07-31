#include "modules/supervisor.h"
#include "hal/system.h"
#include "hal/watchdog.h"
#include "tasks/supervisor.h"

using namespace teller::hal;

// Design guidelines for the supervisor task:
// http://www.ganssle.com/watchdogs.htm
// section "Watchdog Timers for Multitasking"

[[noreturn]] void teller::tasks::supervisorTask(void* arg)
{
    watchdog::configureAndStart();

    // TODO: implement the guidelines outlined above; this is just a placeholder
    // for the time being
    for (;;) {
        system::delayMsec(1000);
        supervisor::checkTasks();
        watchdog::reset();
    }
}
