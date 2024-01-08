#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <task.h>

#include "hal/hal.h"
#include "modules/errors.h"
#include "tasks/blinker.h"
#include "tasks/serial.h"

int main(void)
{
    bool inited;

    osKernelInitialize();

    inited = teller::hal::init();

    /* We start the error handler module no matter what. In the worst case, the
     * error LED won't work but at least we are still logging the errors */
    teller::errors::init();

    /* The remaining tasks are started only if the HAL initialization was successful */
    if (inited) {
        osThreadNew(teller::tasks::blinkTask, NULL, &teller::tasks::blinkTaskAttr);
        osThreadNew(teller::tasks::serialTask, NULL, &teller::tasks::serialTaskAttr);
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
