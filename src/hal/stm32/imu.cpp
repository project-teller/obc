#include "hal/imu.h"
#include "config.h"
#include "hal/system.h"

using namespace teller::hal::system;

namespace teller::hal::imu {

bool init()
{
    return true;
}

void destroy()
{
}

bool getAcceleration(measurement_t& result)
{
    return false;
}

bool getAngularVelocity(measurement_t& result)
{
    return false;
}

bool update()
{
    sleepForever();
    return false;
}

}
