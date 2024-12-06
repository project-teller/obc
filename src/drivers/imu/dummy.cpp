#include "config.h"
#include "drivers/imu.h"
#include "hal/system.h"

using namespace teller::hal::system;
using teller::telem::measurement_3d_t;

namespace teller::drivers::imu {

bool init()
{
    return true;
}

void destroy()
{
}

bool setup(bool logErrors)
{
    return false;
}

bool update(measurement_3d_t& acceleration, measurement_3d_t& angularVelocity)
{
    sleepForever();
    return false;
}

}
