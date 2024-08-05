#include <cassert>

#include "modules/supervisor.h"
#include "modules/telem.h"
#include "tasks/serial.h"

using namespace teller;

void teller::tasks::serialTask(void* arg)
{
    supervisor::QueueRegistration queue("serial", telem::getQueue());

    while (true) {
        telem::flushNext();
    }
}
