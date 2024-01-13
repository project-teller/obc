#include "tasks/supervisor.h"
#include "hal/led.h"
#include "hal/watchdog.h"

using namespace teller::hal;

// Design guidelines for the supervisor task:
// http://www.ganssle.com/watchdogs.htm
// section "Watchdog Timers for Multitasking"

const osThreadAttr_t teller::tasks::supervisorTaskAttr = {
    .name = "supervisor",
    .priority = osPriorityLow,
};

__NO_RETURN void teller::tasks::supervisorTask(void* arg)
{
    teller::hal::watchdog::init();

    // TODO: implement the guidelines outlined above; this is just a placeholder
    // for the time being
    for (;;) {
        osDelay(1000);
        watchdog::reset();
    }
}
