#include "blinker.h"
#include "hal/led.h"

using namespace teller::hal;

const osThreadAttr_t teller::tasks::blinkTaskAttr = {
    .name = "blinker",
    .priority = osPriorityLow,
};

void teller::tasks::blinkTask(void* arg)
{
    for (;;) {
        led::set(led::HEARTBEAT);
        osDelay(100);
        led::clear(led::HEARTBEAT);
        osDelay(100);
        led::set(led::HEARTBEAT);
        osDelay(100);
        led::clear(led::HEARTBEAT);
        osDelay(700);
    }
}
