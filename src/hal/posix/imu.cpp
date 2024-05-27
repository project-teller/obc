#include "core/utils/random.h"

#include "hal/imu.h"
#include "hal/system.h"

#include "config.h"

using namespace teller::hal;

namespace teller::hal::imu {

/**
 * @def UPDATE_FREQUENCY_HZ
 * @brief Update frequency of the simulated IMU.
 */
#define UPDATE_FREQUENCY_HZ 100

/**
 * @def ACCEL_NOISE
 * Standard deviation of the accelerometer measurement noise.
 */
#define ACCEL_NOISE 0.1f

/**
 * @def GYRO_NOISE
 * Standard deviation of the angular velocity measurement noise.
 */
#define GYRO_NOISE 0.1f

static measurement_t acceleration;
static measurement_t angular_velocity;

bool init()
{
    return true;
}

void destroy()
{
}

bool getAcceleration(measurement_t& result)
{
    bool updated = result.timestampInMsec != acceleration.timestampInMsec;
    result = acceleration;
    return updated;
}

bool getAngularVelocity(measurement_t& result)
{
    bool updated = result.timestampInMsec != angular_velocity.timestampInMsec;
    result = angular_velocity;
    return updated;
}

bool update()
{
    uint32_t now;

    /* The simulated IMU provides new measurements at 100 Hz */
    system::delayMsec(1000 / UPDATE_FREQUENCY_HZ);

    now = system::getTimeSinceBootMsec();

    acceleration.timestampInMsec = now;
    acceleration.x = rng_gauss() * ACCEL_NOISE;
    acceleration.y = rng_gauss() * ACCEL_NOISE;
    acceleration.z = -9.81f + rng_gauss() * ACCEL_NOISE;

    angular_velocity.timestampInMsec = now;
    angular_velocity.x = rng_gauss() * GYRO_NOISE;
    angular_velocity.y = rng_gauss() * GYRO_NOISE;
    angular_velocity.z = rng_gauss() * GYRO_NOISE;

    return true;
}

}
