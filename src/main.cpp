#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <task.h>

#include "hal/hal.h"
#include "modules/errors.h"
#include "modules/telem.h"
#include "tasks/blinker.h"
#include "tasks/pins.h"
#include "tasks/serial.h"
#include "tasks/supervisor.h"
#include "tasks/telem.h"

namespace teller::tasks {

const osThreadAttr_t blinkTaskAttr = {
    .name = "blinker",
    .priority = osPriorityLow,
};

const osThreadAttr_t pinsTaskAttr = {
    .name = "pins",
    .priority = osPriorityNormal,
};

const osThreadAttr_t serialTaskAttr = {
    .name = "serial",
    .stack_size = 1024,
    .priority = osPriorityHigh,
};

const osThreadAttr_t supervisorTaskAttr = {
    .name = "supervisor",
    .priority = osPriorityLow,
};

const osThreadAttr_t telemetryTaskAttr = {
    .name = "telem",
    .stack_size = 1024,
    .priority = osPriorityNormal,
};

}

int main(void)
{
    bool inited;

    osKernelInitialize();

    inited = teller::hal::init();

    /* We start the error handler module no matter what. In the worst case, the
     * error LED won't work but at least we are still logging the errors */
    teller::errors::init();

    inited &= teller::telem::init();

    /* The remaining tasks are started only if the HAL initialization was successful */
    if (inited) {
        osThreadNew(teller::tasks::blinkTask, nullptr, &teller::tasks::blinkTaskAttr);
        osThreadNew(teller::tasks::pinsTask, nullptr, &teller::tasks::pinsTaskAttr);
        osThreadNew(teller::tasks::serialTask, nullptr, &teller::tasks::serialTaskAttr);
        osThreadNew(teller::tasks::supervisorTask, nullptr, &teller::tasks::supervisorTaskAttr);
        osThreadNew(teller::tasks::telemetryTask, nullptr, &teller::tasks::telemetryTaskAttr);
    } else {
        teller::errors::setError(teller::errors::SYSTEM_INIT_ERROR);
    }

    osKernelStart();

    for (;;)
        ;

    return 0;
}

extern "C" void vApplicationTickHook(void)
{
}

extern "C" void vApplicationIdleHook(void)
{
}

extern "C" void vApplicationMallocFailedHook(void)
{
    /* TODO: store the event in a .noinit variable so we can report it at next
     * boot. Maybe also auto-reset? */
    teller::hal::notifyFatalError();

    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;

    /* TODO: store the event in a .noinit variable so we can report it at next
     * boot. Maybe also auto-reset? */

    teller::hal::notifyFatalError();

    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}
