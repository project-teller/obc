#include <cassert>

#include "modules/telem.h"
#include "tasks/serial.h"

using namespace teller;

void teller::tasks::serialTask(void* arg)
{
    while (true) {
        telem::flushNext();
    }
}
