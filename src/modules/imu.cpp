#include <algorithm>

#include "hal/imu.h"
#include "hal/system.h"

#include "modules/edr.hpp"
#include "modules/imu.h"

using namespace teller::hal;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;
static imu::measurement_t acceleration;
static imu::measurement_t angular_velocity;

static teller::edr::FormattedLogRecord<uint32_t, uint8_t, float, float, float, float, float, float>
    logRecord(2, "IMU", "TimeMS,I,AccX,AccY,AccZ,GyrX,GyrY,GyrZ", "IBffffff", "s#EEEooo"); // multipliers: C-000000

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

void log()
{
    bool changed = false;

    changed |= teller::hal::imu::getAcceleration(acceleration);
    changed |= teller::hal::imu::getAngularVelocity(angular_velocity);

    if (changed) {
        logRecord.write(
            std::max(acceleration.time_msec, angular_velocity.time_msec),
            0, /* first IMU */
            acceleration.x, acceleration.y, acceleration.z,
            angular_velocity.x, angular_velocity.y, angular_velocity.z);
    }
}

}
