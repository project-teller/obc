#include <algorithm>

#include "core/math/running_mean.hpp"
#include "core/math/vector.hpp"

#include "hal/imu.h"
#include "hal/system.h"

#include "modules/edr.hpp"
#include "modules/imu.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::math;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;

static measurement_3d_t acceleration;
static measurement_3d_t rawAngularVelocity;
static measurement_3d_t angularVelocity;

/** Gyro offset used for compensation after calibration */
static Vector3f gyroOffset;

static teller::edr::FormattedLogRecord<uint32_t, uint8_t, float, float, float, float, float, float>
    logRecord(
        2, "IMU", "TimeMS,I,AccX,AccY,AccZ,GyrX,GyrY,GyrZ",
        "IBffffff", "s#EEEooo", "C-000000");

/**
 * @brief Class representing the status of the gyro calibration.
 *
 */
class GyroCalibration {

public:
    GyroCalibration()
        : mean()
        , running(false)
        , startedAt(0)
    {
    }

    /**
     * @brief Starts or restarts the calibration process of the gyroscope.
     */
    void start()
    {
        mean.reset();
        running = true;
        startedAt = teller::hal::system::getTimeSinceBootMsec();
    }

    /**
     * @brief Feeds a new raw measurement into the calibration process.
     *
     * @return Whether the calibration process has ended.
     */
    bool feedMeasurement(const measurement_3d_t& measurement)
    {
        bool ended;

        mean.add(measurement.value);

        // We need at least 50 samples and at least 1 second of measurement
        ended = mean.countSamples() >= 50 && (measurement.timestampInMsec - startedAt) >= 1000;

        if (ended) {
            running = false;
        }

        return ended;
    }

    /**
     * @brief Returns the estimated gyro offset.
     */
    void getOffset(Vector3f& result) const
    {
        result = mean.get();
    }

    /**
     * @brief Returns whether the calibration process is running.
     */
    bool isRunning()
    {
        return running;
    }

private:
    RunningMean<Vector3f> mean;
    bool running;
    uint32_t startedAt;
};

static GyroCalibration gyroCalibration;
static Logger* logger;

namespace teller::imu {

bool init()
{
    status = SUBSYSTEM_STATUS_CRITICAL;
    logger = getLogger(MODULE_ID_IMU);
    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
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

void startGyroCalibration(void)
{
    if (logger != nullptr) {
        logger->info("Starting gyro calibration.");
    }
    gyroCalibration.start();
}

bool update()
{
    status = teller::hal::imu::update(acceleration, rawAngularVelocity)
        ? SUBSYSTEM_STATUS_OK
        : SUBSYSTEM_STATUS_ERROR;

    if (gyroCalibration.isRunning() && gyroCalibration.feedMeasurement(rawAngularVelocity)) {
        /* Calibration ended, store the offset */
        gyroCalibration.getOffset(gyroOffset);
        if (logger != nullptr) {
            logger->info("Gyro calibrated %f.", gyroOffset.y);
        }
    }

    /* Apply gyro offset to measurement */
    angularVelocity.timestampInMsec = rawAngularVelocity.timestampInMsec;
    angularVelocity.value = rawAngularVelocity.value - gyroOffset;

    return status == SUBSYSTEM_STATUS_OK;
}

void log()
{
    logRecord.write(
        std::max(acceleration.timestampInMsec, angularVelocity.timestampInMsec),
        0, /* first IMU */
        acceleration.value.x, acceleration.value.y, acceleration.value.z,
        angularVelocity.value.x, angularVelocity.value.y, angularVelocity.value.z);
}

}
