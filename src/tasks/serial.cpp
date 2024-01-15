#include <cassert>

#include "modules/telem.h"
#include "tasks/serial.h"

using namespace teller;

const osThreadAttr_t teller::tasks::serialTaskAttr = {
    .name = "serial",
    .stack_size = 1024,
    .priority = osPriorityHigh,
};

void teller::tasks::serialTask(void* arg)
{
    while (true) {
        telem::flushNext();
    }
}
