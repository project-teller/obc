#include <algorithm>

#include "core/log_records.h"
#include "core/math/vector.hpp"

#include "drivers/mag.h"
#include "hal/system.h"

#include "modules/edr.hpp"
#include "modules/log.h"
#include "modules/mag.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::math;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;

static measurement_3d_t rawMagneticVector;
static measurement_3d_t magneticVector;
static float temperature = 0.0f;

static teller::edr::FormattedLogRecord<uint32_t, uint8_t, float, float, float, int8_t>
    logRecord(
        LOG_RECORD_MAG, "MAG",
        "TimeMS,I,MagX,MagY,MagZ,Temp",
        "IBfffb", "s#GGGO", "C-CCCA");

static void convertFromSensorToBodyFrame(Vector3f& sensor, Vector3f& body);

static Logger* logger;

namespace teller::mag {

bool init()
{
    status = SUBSYSTEM_STATUS_CRITICAL;
    logger = getLogger(MODULE_ID_MAG);
    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
    status = SUBSYSTEM_STATUS_CRITICAL;
}

measurement_3d_t getMagneticVector(void)
{
    return magneticVector;
}

float getTemperature(void)
{
    return temperature;
}

subsystem_status_t getSubsystemStatus()
{
    return status;
}

bool setup()
{
    status = teller::drivers::mag::setup()
        ? SUBSYSTEM_STATUS_OK
        : SUBSYSTEM_STATUS_ERROR;
    return status == SUBSYSTEM_STATUS_OK;
}

bool update()
{
    status = teller::drivers::mag::update(rawMagneticVector, temperature)
        ? SUBSYSTEM_STATUS_OK
        : SUBSYSTEM_STATUS_ERROR;

    magneticVector.timestampInMsec = rawMagneticVector.timestampInMsec;
    convertFromSensorToBodyFrame(rawMagneticVector.value, magneticVector.value);

    return status == SUBSYSTEM_STATUS_OK;
}

void log()
{
    static uint32_t lastTimestamp = 0;

    if (magneticVector.timestampInMsec == lastTimestamp) {
        // No new measurement
        return;
    }

    lastTimestamp = magneticVector.timestampInMsec;
    logRecord.write(
        magneticVector.timestampInMsec,
        0, /* first magnetometer */
        magneticVector.value.x, magneticVector.value.y, magneticVector.value.z,
        static_cast<int8_t>(temperature * 10));
}

}

/**
 * @brief Converts a measurement from the sensor frame to the body frame.
 *
 * The conversion still needs to be worked out. Until then, this function
 * simply copies the input to the output.
 *
 * @param sensor  The measurement in the sensor frame.
 * @param body    The measurement in the body frame.
 */
static void convertFromSensorToBodyFrame(Vector3f& sensor, Vector3f& body)
{
    body.x = sensor.x;
    body.y = sensor.y;
    body.z = sensor.z;
}
