#include "config.h"
#include "drivers/mag.h"
#include "hal/system.h"

using namespace teller::hal::system;
using teller::telem::measurement_3d_t;

namespace teller::drivers::mag {

bool init()
{
    return true;
}

void destroy()
{
}

bool setup()
{
    return false;
}

bool update(measurement_3d_t& magneticVector)
{
    sleepForever();
    return false;
}

}
