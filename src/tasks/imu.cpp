#include "modules/imu.h"
#include "tasks/imu.h"

[[noreturn]] void teller::tasks::imuTask(void* arg)
{
    while (true) {
        teller::imu::update();
    }
}
