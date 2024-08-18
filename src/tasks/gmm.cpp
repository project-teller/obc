#include "tasks/gmm.h"
#include "hal/system.h"
#include "modules/gmm.h"

using namespace teller;

[[noreturn]] void teller::tasks::gmmTask(void* arg)
{
    bool healthy = gmm::setup();
    while (healthy) {
        healthy = gmm::update();
    }

    /* TODO: maybe reset and retry? */
    hal::system::sleepForever();
}
