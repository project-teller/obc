#include "modules/imu.h"
#include "hal/system.h"
#include "tasks/imu.h"

using namespace teller;

[[noreturn]] void teller::tasks::imuTask(void* arg)
{
    bool healthy;

    while (true) {
        healthy = imu::setup();
        while (healthy) {
            healthy = imu::update();
        }

        /* IMU not healthy, wait a bit and then retry */
        hal::system::delayMsec(1000);
    }
}
