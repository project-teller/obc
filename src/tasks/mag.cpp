#include "modules/mag.h"
#include "hal/system.h"
#include "tasks/mag.h"

using namespace teller;

[[noreturn]] void teller::tasks::magTask(void* arg)
{
    bool healthy = mag::setup();
    while (healthy) {
        healthy = mag::update();
    }

    /* TODO: maybe reset and retry? */
    hal::system::sleepForever();
}
