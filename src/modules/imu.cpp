#include <algorithm>

#include "core/log_records.h"
#include "core/math/running_mean.hpp"
#include "core/math/vector.hpp"

#include "drivers/imu.h"
#include "hal/system.h"

#include "modules/edr.hpp"
#include "modules/imu.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::math;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;

static measurement_3d_t rawAcceleration;
static measurement_3d_t acceleration;
static measurement_3d_t rawAngularVelocity;
static measurement_3d_t angularVelocity;

/** Gyro offset used for compensation after calibration */
static Vector3f gyroOffset;

static teller::edr::FormattedLogRecord<uint32_t, uint8_t, float, float, float, float, float, float>
    logRecord(
        LOG_RECORD_IMU, "IMU",
        "TimeMS,I,AccX,AccY,AccZ,GyrX,GyrY,GyrZ",
        "IBffffff", "s#EEEooo", "C-000000");

static void convertFromSensorToBodyFrame(Vector3f& sensor, Vector3f& body);

/**
 * @brief Class representing the status of the gyro calibration.
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

/**
 * @brief Class representing the status of the accelerometer calibration.
 */
class AccelCalibration {

private:
    enum class State {
        NOT_RUNNING = 0,
        STARTED = 1,
        WAITING_FOR_REST = 2,
        SAMPLING = 3,
        FINISHED = 4
    };

    enum class Side : uint8_t {
        NONE = 0,
        X_NEG = 1,
        X_POS = 2,
        Y_NEG = 4,
        Y_POS = 8,
        Z_NEG = 16,
        Z_POS = 32,
        ALL = X_NEG | X_POS | Y_NEG | Y_POS | Z_NEG | Z_POS
    };

public:
    AccelCalibration()
        : mean()
        , state(State::NOT_RUNNING)
        , lastStateChangeAt(0)
    {
    }

    /**
     * @brief Cancels the calibration process of the accelerometer.
     */
    void cancel()
    {
        setState(State::NOT_RUNNING);
    }

    /**
     * @brief Starts or restarts the calibration process of the accelerometer.
     */
    void start()
    {
        setState(State::STARTED);
    }

    /**
     * @brief Feeds a new raw measurement into the calibration process.
     *
     * @return Whether the calibration process has ended.
     */
    bool feedMeasurement(const measurement_3d_t& measurement)
    {
        Side side;

#define HAVE_ENOUGH_SAMPLES() (mean.countSamples() >= 50 && (measurement.timestampInMsec - lastStateChangeAt) >= 1000)

        switch (state) {
        case State::NOT_RUNNING:
            /* Nothing to do */
            break;

        case State::STARTED:
            /* Drop sample, move unconditionally to next state */
            setState(State::WAITING_FOR_REST);
            break;

        case State::WAITING_FOR_REST:
            side = identifySide(measurement.value);
            if (expectedSide == Side::NONE) {
                expectedSide = side;
            } else if (expectedSide == side) {
                // We need at least 50 samples and at least 1 second of measurement
                mean.add(measurement.value); // just counting, we don't care about the value
                if (HAVE_ENOUGH_SAMPLES()) {
                    setState(State::SAMPLING);
                }
            } else {
                // Device is not resting, restart this state
                setState(State::STARTED);
            }
            break;

        case State::SAMPLING:
            side = identifySide(measurement.value);
            if (expectedSide == side) {
                // Keep the sample
                mean.add(measurement.value);
                if (HAVE_ENOUGH_SAMPLES()) {
                    if (updateExtrema()) {
                        previousFinishedSide = side;
                        finishedSides |= static_cast<uint8_t>(side);
                        if (finishedSides == static_cast<uint8_t>(Side::ALL)) {
                            setState(State::FINISHED);
                        } else {
                            setState(State::STARTED);
                        }
                    } else {
                        setState(State::STARTED);
                    }
                }
            } else {
                // Not resting
                setState(State::STARTED);
            }
            break;

        case State::FINISHED:
            /* Nothing to do */
            break;

        default:
            /* Unknown state, cancel calibration */
            setState(State::NOT_RUNNING);
        }

        return !isRunning();
    }

    /**
     * @brief Returns the estimated accelerometer offsets.
     */
    void getOffset(Vector3f& result) const
    {
        result = (maxs + mins).elementwiseDiv(mins - maxs);
    }

    /**
     * @brief Returns the estimated accelerometer scaling factors.
     */
    void getScaling(Vector3f& result) const
    {
        result = maxs - mins;
        result.x = 2 / result.x;
        result.y = 2 / result.y;
        result.z = 2 / result.z;
    }

    /**
     * @brief Returns whether the calibration process is running.
     */
    bool isRunning()
    {
        return state != State::NOT_RUNNING && state != State::FINISHED;
    }

private:
    RunningMean<Vector3f> mean;
    Vector3f mins;
    Vector3f maxs;
    State state;
    Side expectedSide;
    Side previousFinishedSide;
    uint8_t finishedSides;
    uint32_t lastStateChangeAt;

    /**
     * @brief Identifies which side the device is most likely to lie on.
     */
    Side identifySide(const Vector3f& measurement)
    {
        Vector3f absMeasurement = measurement.abs();
        int maxIndex = absMeasurement.argmax();
        if (2 * absMeasurement[maxIndex] > (absMeasurement[0] + absMeasurement[1] + absMeasurement[2])) {
            // There is a dominant side
            if (maxIndex == 0) {
                return measurement[maxIndex] > 0 ? Side::X_POS : Side::X_NEG;
            } else if (maxIndex == 1) {
                return measurement[maxIndex] > 0 ? Side::Y_POS : Side::Y_NEG;
            } else {
                return measurement[maxIndex] > 0 ? Side::Z_POS : Side::Z_NEG;
            }
        } else {
            // No dominant side
            return Side::NONE;
        }
    }

    /**
     * @brief Sets the state of the algorithm and adjusts other state variables
     * according to the state transition.
     */
    void setState(State state)
    {
        if (this->state == state) {
            return;
        }

        this->state = state;
        this->lastStateChangeAt = (state == State::NOT_RUNNING
                ? 0
                : teller::hal::system::getTimeSinceBootMsec());

        switch (state) {
        case State::NOT_RUNNING:
            mean.reset();
            previousFinishedSide = Side::NONE;
            finishedSides = static_cast<uint8_t>(Side::NONE);
            mins *= 0;
            maxs *= 0;
            break;

        case State::STARTED:
        case State::WAITING_FOR_REST:
            mean.reset();
            expectedSide = Side::NONE;
            break;

        case State::SAMPLING:
            mean.reset();
            break;

        case State::FINISHED:
            expectedSide = Side::NONE;
            break;
        }
    }

    bool updateExtrema()
    {
        switch (expectedSide) {
        case Side::X_POS:
            maxs[0] = mean.get()[0];
            return true;

        case Side::X_NEG:
            mins[0] = mean.get()[0];
            return true;

        case Side::Y_POS:
            maxs[1] = mean.get()[1];
            return true;

        case Side::Y_NEG:
            mins[1] = mean.get()[1];
            return true;

        case Side::Z_POS:
            maxs[2] = mean.get()[2];
            return true;

        case Side::Z_NEG:
            mins[2] = mean.get()[2];
            return true;

        default:
            return false;
        }
    }
};

static AccelCalibration accelCalibration;
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

bool setup(bool logErrors)
{
    status = teller::drivers::imu::setup(logErrors)
        ? SUBSYSTEM_STATUS_OK
        : SUBSYSTEM_STATUS_ERROR;
    return status == SUBSYSTEM_STATUS_OK;
}

void startAccelerometerCalibration(void)
{
    if (logger != nullptr) {
        logger->info("Starting accelerometer calibration.");
    }
    accelCalibration.start();
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
    status = teller::drivers::imu::update(rawAcceleration, rawAngularVelocity)
        ? SUBSYSTEM_STATUS_OK
        : SUBSYSTEM_STATUS_ERROR;

    if (gyroCalibration.isRunning() && gyroCalibration.feedMeasurement(rawAngularVelocity)) {
        /* Gyro calibration ended, store the offset */
        gyroCalibration.getOffset(gyroOffset);
        if (logger != nullptr) {
            logger->info("Gyro calibrated");
        }
    }

    if (accelCalibration.isRunning() && accelCalibration.feedMeasurement(rawAcceleration)) {
        /* Accel calibration ended */
        /* TODO(ntamas): store the offset and scaling */
        // gyroCalibration.getOffset(gyroOffset);
        if (logger != nullptr) {
            logger->info("Accelerometer calibrated");
        }
    }

    /* Convert raw measurements from IMU frame to body frame */
    acceleration.timestampInMsec = rawAcceleration.timestampInMsec;
    angularVelocity.timestampInMsec = rawAngularVelocity.timestampInMsec;
    convertFromSensorToBodyFrame(rawAcceleration.value, acceleration.value);
    convertFromSensorToBodyFrame(rawAngularVelocity.value, angularVelocity.value);

    /* Apply gyro offset to measurement */
    angularVelocity.value = rawAngularVelocity.value - gyroOffset;

    return status == SUBSYSTEM_STATUS_OK;
}

void log()
{
    static uint32_t lastTimestamp = 0;

    if (acceleration.timestampInMsec == lastTimestamp) {
        // No new measurement
        return;
    }

    lastTimestamp = acceleration.timestampInMsec;
    logRecord.write(
        std::max(acceleration.timestampInMsec, angularVelocity.timestampInMsec),
        0, /* first IMU */
        acceleration.value.x, acceleration.value.y, acceleration.value.z,
        angularVelocity.value.x, angularVelocity.value.y, angularVelocity.value.z);
}

}

/**
 * @brief Converts a measurement from the sensor frame to the body frame.
 *
 * In the REXUS coordinate system, X is the longitudinal axis (positive up),
 * Z points away from the launcher rail, and Y is oriented such that the XYZ
 * axes form a right-handed coordinate system.
 *
 * On the IMU, the X and the Y axes are in the plane of the sensor and the
 * Z axis points out of the sensor, away from the PCB.
 *
 * In a Blender model, it can be worked out that all axes need to be inverted
 * and the X and Y axes have to be swapped to convert from the sensor frame to
 * the body frame or vice versa.
 *
 * @param sensor  The measurement in the sensor frame.
 * @param body    The measurement in the body frame.
 */
static void convertFromSensorToBodyFrame(Vector3f& sensor, Vector3f& body)
{
    body.x = -sensor.y;
    body.y = -sensor.x;
    body.z = -sensor.z;
}
