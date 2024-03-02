#include "hal/led.h"
#include "tasks/blinker.h"

using namespace teller::hal;

const osThreadAttr_t teller::tasks::pinsTaskAttr = {
    .name = "pins",
    .priority = osPriorityNormal,
};

__NO_RETURN void teller::tasks::pinsTask(void* arg)
{
    for (;;) {
        osDelay(20);
    }
}
