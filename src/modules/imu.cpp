#include "hal/imu.h"
#include "hal/system.h"

#include "modules/imu.h"

using namespace teller::hal;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;

namespace teller::imu {

bool init()
{
    status = SUBSYSTEM_STATUS_CRITICAL;
    return true;
}

void destroy()
{
    status = SUBSYSTEM_STATUS_CRITICAL;
}

subsystem_status_t getSubsystemStatus()
{
    return status;
}

void update()
{
    status = teller::hal::imu::update()
        ? SUBSYSTEM_STATUS_OK
        : SUBSYSTEM_STATUS_ERROR;
}

}
