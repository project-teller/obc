#include "tasks/serial.h"
#include "hal/uart.h"

using namespace teller::hal;

const osThreadAttr_t teller::tasks::serialTaskAttr = {
    .name = "serial",
    .priority = osPriorityLow,
};

__NO_RETURN void teller::tasks::serialTask(void* arg)
{
    for (;;) {
        uart::write(uart::TELEMETRY, "hello\n");
        osDelay(700);
    }
}
