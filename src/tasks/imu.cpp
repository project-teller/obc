#include "modules/imu.h"
#include "hal/system.h"
#include "tasks/imu.h"

using namespace teller;

[[noreturn]] void teller::tasks::imuTask(void* arg)
{
    bool healthy = imu::setup();
    while (healthy) {
        healthy = imu::update();
    }

    /* TODO: maybe reset and retry? */
    hal::system::sleepForever();
}
