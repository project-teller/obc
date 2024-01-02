#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <task.h>

#include "hal/hal.h"
#include "tasks/blinker.h"
#include "tasks/serial.h"

int main(void)
{
    osKernelInitialize();

    if (teller::hal::init()) {
        osThreadNew(teller::tasks::blinkTask, NULL, &teller::tasks::blinkTaskAttr);
        osThreadNew(teller::tasks::serialTask, NULL, &teller::tasks::serialTaskAttr);

        osKernelStart();

        /* should not ever get here */
    }

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
    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;

    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}
