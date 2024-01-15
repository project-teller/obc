#include <cstring>

#include "modules/telem.h"
#include "tasks/serial.h"
#include "tasks/telem.h"

using namespace teller::telem;

const osThreadAttr_t teller::tasks::telemetryTaskAttr = {
    .name = "telem",
    .stack_size = 1024,
    .priority = osPriorityNormal,
};

__NO_RETURN void teller::tasks::telemetryTask(void* arg)
{
    uint8_t msg[] = { 0xde, 0xad, 0xbe, 0xef };
    for (;;) {
        send(frames::TEXT_MESSAGE, msg, sizeof(msg));
        osDelay(700);
    }
}
