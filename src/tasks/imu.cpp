#include "modules/imu.h"
#include "hal/system.h"
#include "tasks/imu.h"

using namespace teller;

[[noreturn]] void teller::tasks::imuTask(void* arg)
{
    bool healthy, logErrors = true;

    while (true) {
        healthy = imu::setup(logErrors);
        while (healthy) {
            logErrors = true;
            healthy = imu::update();
        }

        /* IMU not healthy, wait a bit and then retry */
        logErrors = false;
        hal::system::delayMsec(1000);
    }
}
