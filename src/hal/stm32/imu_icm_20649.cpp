#include <cstring>
#include <limits>

#include "config.h"
#include "hal/imu.h"
#include "hal/spi.h"
#include "hal/system.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

static const spi::address_t imu_address = {
    .bus = 0,
    .device = 0
};

/* Registers in user bank 0 */
#define REG_WHO_AM_I 0x00
#define REG_PWR_MGMT_1 0x06
#define REG_ACCEL_XOUT_H 0x2D
#define REG_GYRO_XOUT_H 0x33
#define REG_BANK_SEL 0x7F

/* Registers in user bank 2 */
#define REG_GYRO_SMPLRT_DIV 0x00
#define REG_GYRO_CONFIG_1 0x01
#define REG_ACCEL_SMPLRT_DIV_1 0x10
#define REG_ACCEL_SMPLRT_DIV_2 0x11
#define REG_ACCEL_CONFIG 0x14

#define READ_REGISTER(x) (static_cast<uint8_t>(x | 0x80))
#define WRITE_REGISTER(x) (x)

[[nodiscard]] static bool readRegisterByte(uint8_t index, uint8_t& result);
[[nodiscard]] static bool writeRegisterByte(uint8_t index, uint8_t value, bool verify = false);

/**
 * Struct containing pairs of register indices and their corresponding values
 * for configuring the IMU.
 */
typedef struct {
    uint8_t bank;
    uint8_t reg;
    uint8_t value;
} configuration_entry_t;

#define NO_MORE_ENTRIES \
    {                   \
        0, 0, 0         \
    }

#define GYRO_SCALE (4000.0f / std::numeric_limits<int16_t>::max())
#define ACCEL_SCALE (30.0f * 9.81f / std::numeric_limits<int16_t>::max())

static const configuration_entry_t config[] = {
    /* Wake up from sleep mode (!0x40), disable temperature sensor (0x08),
     * select best clock source (0x01) */
    { 0, REG_PWR_MGMT_1, 0x09 },

    /* Enable gyro lowpass filter (0x01), full scale = 4000 dps (0x06),
     * lowpass filter 3dB bandwidth at 23.9 Hz (0x20), NBW at 35.9 Hz */
    { 2, REG_GYRO_CONFIG_1, 0x27 },

    /* Gyro sample rate divider = 14 = 0x0E; this yields a sample rate of
     * 1125 / (14+1) = 75 Hz, approximately twice the NBW of the LPF */
    { 2, REG_GYRO_SMPLRT_DIV, 0x0e },

    /* Enable accelerometer lowpass filter (0x01), full scale = 30g (0x06),
     * lowpass filter 3dB bandwidth at 23.9 Hz (0x20), NBW at 35.9 Hz */
    { 2, REG_ACCEL_CONFIG, 0x27 },

    /* Accelerometer sample rate divider = 14 = 0x0E; this yields a sample rate
     * of 1125 / (14+1) = 75 Hz, approximately twice the NBW of the LPF */
    { 2, REG_ACCEL_SMPLRT_DIV_1, 0x00 },
    { 2, REG_ACCEL_SMPLRT_DIV_2, 0x0e },

    NO_MORE_ENTRIES
};

/**
 * @brief Configures the IMU from scratch.
 *
 * @return whether the configuration succeeded.
 */
static bool configure(void);

static Logger* logger;

namespace teller::hal::imu {

bool init()
{
    /* Most of the initialization is done in setup() because we need to run
     * SPI transfers with interrupts */
    logger = getLogger(MODULE_ID_IMU);
    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
}

bool setup()
{
    uint8_t value;

    /* Read WHO_AM_I register, check expected value */
    if (!readRegisterByte(REG_WHO_AM_I, value) || value != 0xe1) {
        logger->error("IMU not found");
        return false;
    } else {
        /* Run configuration from scratch */
        return configure();
    }
}

bool update(measurement_3d_t& acceleration, measurement_3d_t& angularVelocity)
{
    std::uint8_t buf[13] = { 0x80 + REG_ACCEL_XOUT_H };
    std::uint32_t now;

    /* TODO: periodically check configuration registers */

    /* TODO: timing should be better */
    system::delayMsec(20);

    /* Read accelerometer and gyro measurements */
    now = system::getTimeSinceBootMsec();
    if (!spi::transfer(imu_address, buf, sizeof(buf))) {
        logger->error("SPI transfer failed");
        return false;
    }

    /* TODO(ntamas): lock for atomic modification? */
    acceleration.timestampInMsec = now;
    acceleration.value.set(
        static_cast<std::int8_t>((buf[1] << 8) + buf[2]) * ACCEL_SCALE,
        static_cast<std::int8_t>((buf[3] << 8) + buf[4]) * ACCEL_SCALE,
        static_cast<std::int8_t>((buf[5] << 8) + buf[6]) * ACCEL_SCALE);

    angularVelocity.timestampInMsec = now;
    angularVelocity.value.set(
        static_cast<std::int8_t>((buf[7] << 8) + buf[8]) * GYRO_SCALE,
        static_cast<std::int8_t>((buf[9] << 8) + buf[10]) * GYRO_SCALE,
        static_cast<std::int8_t>((buf[11] << 8) + buf[12]) * GYRO_SCALE);

    return true;
}
}

static bool readRegisterByte(uint8_t index, uint8_t& result)
{
    uint8_t buf[] = { READ_REGISTER(index), 0x00 };
    if (!spi::transfer(imu_address, buf, sizeof(buf))) {
        return false;
    }
    result = buf[1];
    return true;
}

static bool writeRegisterByte(uint8_t index, uint8_t value, bool verify)
{
    uint8_t buf[] = { WRITE_REGISTER(index), value };
    if (!spi::transfer(imu_address, buf, sizeof(buf))) {
        return false;
    }

    if (verify) {
        uint8_t observed;
        if (!readRegisterByte(index, observed) || observed != value) {
            return false;
        }
    }

    return true;
}

static bool configure()
{
    const configuration_entry_t* entry = config;
    uint8_t bank = 0xff;

    while (entry->bank != 0 || entry->reg != 0) {
        /* Switch register bank if needed */
        if (entry->bank != bank) {
            if (!writeRegisterByte(REG_BANK_SEL, (entry->bank & 0x03) << 4, /* verify = */ true)) {
                logger->error("Switch to bank %d failed", entry->bank & 0x03);
                return false;
            }

            bank = entry->bank;
        }

        /* Write to configuration register */
        if (!writeRegisterByte(entry->reg, entry->value, /* verify = */ true)) {
            logger->error("Setting IMU reg %d in bank %d failed", entry->reg, bank);
            return false;
        }

        entry++;
    }

    /* Switch back to bank 0 */
    if (bank != 0) {
        if (!writeRegisterByte(REG_BANK_SEL, 0, /* verify = */ true)) {
            logger->error("Switch to bank %d failed", 0);
            return false;
        }
    }

    return true;
}
