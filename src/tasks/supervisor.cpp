#include "tasks/supervisor.h"
#include "hal/led.h"
#include "hal/system.h"
#include "hal/watchdog.h"

using namespace teller::hal;

// Design guidelines for the supervisor task:
// http://www.ganssle.com/watchdogs.htm
// section "Watchdog Timers for Multitasking"

[[noreturn]] void teller::tasks::supervisorTask(void* arg)
{
    watchdog::configure_and_start();

    // TODO: implement the guidelines outlined above; this is just a placeholder
    // for the time being
    for (;;) {
        system::delayMsec(1000);
        watchdog::reset();
    }
}
