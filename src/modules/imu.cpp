#include <algorithm>

#include "hal/imu.h"
#include "hal/system.h"

#include "modules/edr.hpp"
#include "modules/imu.h"

using namespace teller::hal;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;
static measurement_3d_t acceleration;
static measurement_3d_t angularVelocity;

static teller::edr::FormattedLogRecord<uint32_t, uint8_t, float, float, float, float, float, float>
    logRecord(
        2, "IMU", "TimeMS,I,AccX,AccY,AccZ,GyrX,GyrY,GyrZ",
        "IBffffff", "s#EEEooo", "C-000000");

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

measurement_3d_t getAcceleration(void)
{
    return acceleration;
}

measurement_3d_t getAngularVelocity(void)
{
    return angularVelocity;
}

subsystem_status_t getSubsystemStatus()
{
    return status;
}

bool setup()
{
    status = teller::hal::imu::setup()
        ? SUBSYSTEM_STATUS_OK
        : SUBSYSTEM_STATUS_ERROR;
    return status == SUBSYSTEM_STATUS_OK;
}

bool update()
{
    status = teller::hal::imu::update(acceleration, angularVelocity)
        ? SUBSYSTEM_STATUS_OK
        : SUBSYSTEM_STATUS_ERROR;
    return status == SUBSYSTEM_STATUS_OK;
}

void log()
{
    logRecord.write(
        std::max(acceleration.timestampInMsec, angularVelocity.timestampInMsec),
        0, /* first IMU */
        acceleration.x, acceleration.y, acceleration.z,
        angularVelocity.x, angularVelocity.y, angularVelocity.z);
}

}
