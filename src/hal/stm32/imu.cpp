#include "hal/imu.h"
#include "config.h"

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
    return false;
}

}
