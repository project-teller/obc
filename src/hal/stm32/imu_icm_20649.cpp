#include <cstring>

#include "config.h"
#include "hal/imu.h"
#include "hal/spi.h"
#include "hal/system.h"

using namespace teller::hal;

static const teller::hal::spi::address_t imu_address = {
    .bus = 0,
    .device = 0
};

static imu::measurement_t acceleration;
static imu::measurement_t angularVelocity;

#define REG_WHO_AM_I 0x00
#define REG_PWR_MGMT_1 0x06
#define REG_ACCEL_XOUT_H 0x2D
#define REG_GYRO_XOUT_H 0x33

#define READ_REGISTER(x) (0x80 | (x))
#define WRITE_REGISTER(x) (x)

namespace teller::hal::imu {

bool init()
{
    /* Most of the initialization is done in setup() because we need to run
     * SPI transfers with interrupts */
    memset(&acceleration, 0, sizeof(measurement_t));
    memset(&angularVelocity, 0, sizeof(measurement_t));
    return true;
}

void destroy()
{
}

bool getAcceleration(measurement_t& result)
{
    result = acceleration;
    return true;
}

bool getAngularVelocity(measurement_t& result)
{
    result = angularVelocity;
    return true;
}

bool setup()
{
    std::uint8_t buf[2] = { READ_REGISTER(REG_WHO_AM_I) };
    bool success = false;

    /* Read WHO_AM_I register, check expected value */
    if (!spi::transfer(imu_address, buf, 2) || buf[1] != 0xe1) {
        goto exit;
    }

    /* Wake up the device from sleep mode, select best clock source */
    buf[0] = WRITE_REGISTER(REG_PWR_MGMT_1);
    buf[1] = 0x01;
    if (!spi::transfer(imu_address, buf, 2)) {
        goto exit;
    }
    buf[0] = READ_REGISTER(REG_PWR_MGMT_1);
    buf[1] = 0x00;
    if (!spi::transfer(imu_address, buf, 2) || buf[1] != 0x01) {
        goto exit;
    }

    success = true;

exit:
    return success;
}

bool update()
{
    std::uint8_t buf[13] = { READ_REGISTER(REG_ACCEL_XOUT_H) };
    std::int16_t rawValue;
    std::uint32_t now;

    /* TODO: timing should be better */
    system::delayMsec(20);

    /* Read accelerometer and gyro measurements */
    now = system::getTimeSinceBootMsec();
    if (!spi::transfer(imu_address, buf, sizeof(buf))) {
        return false;
    }

    /* TODO(ntamas): lock for atomic modification? */
    acceleration.timestampInMsec = now;
    rawValue = (buf[1] << 8) + buf[2];
    acceleration.x = rawValue;
    rawValue = (buf[3] << 8) + buf[4];
    acceleration.y = rawValue;
    rawValue = (buf[5] << 8) + buf[6];
    acceleration.z = rawValue;

    angularVelocity.timestampInMsec = now;
    rawValue = (buf[7] << 8) + buf[8];
    angularVelocity.x = rawValue;
    rawValue = (buf[9] << 8) + buf[10];
    angularVelocity.y = rawValue;
    rawValue = (buf[11] << 8) + buf[12];
    angularVelocity.z = rawValue;

    return true;
}
}
