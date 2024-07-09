#include "core/utils/random.h"

#include "hal/imu.h"
#include "hal/system.h"

#include "config.h"

using namespace teller::hal;
using teller::telem::measurement_3d_t;

static measurement_3d_t gyroOffset;

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

bool init()
{
    /* Generate a random gyro offset to simulate measurement errors that can
     * be compensated for with a calibration procedure */
    gyroOffset.x = (2 * rng_unif01() - 1) * GYRO_NOISE * 100;
    gyroOffset.y = (2 * rng_unif01() - 1) * GYRO_NOISE * 100;
    gyroOffset.z = (2 * rng_unif01() - 1) * GYRO_NOISE * 100;

    return true;
}

void destroy()
{
}

bool setup()
{
    return true;
}

bool update(measurement_3d_t& acceleration, measurement_3d_t& angularVelocity)
{
    uint32_t now;

    /* The simulated IMU provides new measurements at 100 Hz */
    system::delayMsec(1000 / UPDATE_FREQUENCY_HZ);

    now = system::getTimeSinceBootMsec();

    acceleration.timestampInMsec = now;
    acceleration.x = rng_gauss() * ACCEL_NOISE;
    acceleration.y = rng_gauss() * ACCEL_NOISE;
    acceleration.z = -9.81f + rng_gauss() * ACCEL_NOISE;

    angularVelocity.timestampInMsec = now;
    angularVelocity.x = rng_gauss() * GYRO_NOISE + gyroOffset.x;
    angularVelocity.y = rng_gauss() * GYRO_NOISE + gyroOffset.y;
    angularVelocity.z = rng_gauss() * GYRO_NOISE + gyroOffset.z;

    return true;
}

}
