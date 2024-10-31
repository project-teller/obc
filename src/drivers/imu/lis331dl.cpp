#include <cstring>
#include <limits>

#include "config.h"
#include "drivers/imu.h"
#include "hal/spi.h"
#include "hal/system.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

#if defined(TELLER_BOARD_NUCLEO144)
static const spi::address_t address = spi::NO_ADDRESS;
#elif defined(TELLER_BOARD_STM32F4)
/* SPI bus 3, CS pin 3 */
static const spi::address_t address = { .bus = 2, .device = 2 };
#else
static const spi::address_t address = spi::NO_ADDRESS;
#endif

/* Register map */
#define REG_WHO_AM_I 0x0F
#define REG_CTRL_REG1 0x20
#define REG_CTRL_REG2 0x21
#define REG_CTRL_REG3 0x22
#define REG_CTRL_REG4 0x23
#define REG_CTRL_REG5 0x24
#define REG_HP_FILTER_RESET 0x25
#define REG_REFERENCE 0x26
#define REG_STATUS_REG 0x27
#define REG_OUT_X_L 0x28
#define REG_OUT_X_H 0x29
#define REG_OUT_Y_L 0x2A
#define REG_OUT_Y_H 0x2B
#define REG_OUT_Z_L 0x2C
#define REG_OUT_Z_H 0x2D
#define REG_INT1_CFG 0x30
#define REG_INT1_SRC 0x31
#define REG_INT1_THS 0x32
#define REG_INT1_DURATION 0x33
#define REG_INT2_CFG 0x34
#define REG_INT2_SRC 0x35
#define REG_INT2_THS 0x36
#define REG_INT2_DURATION 0x37

#define BIT(x) (1UL << (x))

/* CTRL_REG1 register bits */
#define BIT_XEN BIT(0)
#define BIT_YEN BIT(1)
#define BIT_ZEN BIT(2)
#define BIT_DR0 BIT(3)
#define BIT_DR1 BIT(4)
#define BIT_PM0 BIT(5)
#define BIT_PM1 BIT(6)
#define BIT_PM2 BIT(7)

/* Data rate bit combinations */
#define BITS_DR_50HZ_37HZ 0
#define BITS_DR_100HZ_74HZ BIT_DR0
#define BITS_DR_400HZ_292HZ BIT_DR1
#define BITS_DR_1000HZ_780HZ (BIT_DR0 | BIT_DR1)

/* Power mode combinations */
#define BITS_PM_POWERDOWN 0
#define BITS_PM_NORMAL BIT_PM0
#define BITS_PM_LOW_0HZ_5 BIT_PM1
#define BITS_PM_LOW_1HZ (BIT_PM0 | BIT_PM1)
#define BITS_PM_LOW_2HZ BIT_PM2
#define BITS_PM_LOW_5HZ (BIT_PM2 | BIT_PM0)
#define BITS_PM_LOW_10HZ (BIT_PM2 | BIT_PM1)

/* CTRL_REG4 register bits */
#define BIT_SIM BIT(0)
#define BIT_FS0 BIT(4)
#define BIT_FS1 BIT(5)
#define BIT_BLE BIT(6)
#define BIT_BDU BIT(7)

/* Measurement scale combinations */
#define BITS_FS_100G (0)
#define BITS_FS_200G (BIT_FS0)
#define BITS_FS_400G (BIT_FS0 | BIT_FS1)

#define READ_REGISTER(x) (static_cast<uint8_t>(x | 0x80))
#define READ_REGISTER_AUTO_INCREMENT(x) (static_cast<uint8_t>(x | 0xC0))
#define WRITE_REGISTER(x) (x)

[[nodiscard]] static bool readRegisterByte(uint8_t index, uint8_t& result);
[[nodiscard]] static bool writeRegisterByte(uint8_t index, uint8_t value, bool verify = false);

/**
 * Struct containing pairs of register indices and their corresponding values
 * for configuring the IMU.
 */
typedef struct {
    uint8_t reg;
    uint8_t value;
} configuration_entry_t;

#define NO_MORE_ENTRIES \
    {                   \
        0, 0            \
    }

#define ACCEL_SCALE (100.0f * 9.81f / std::numeric_limits<int16_t>::max())

static const configuration_entry_t config[] = {
    /* Set normal power mode, set data rate and enable all axes */
    { REG_CTRL_REG1, BITS_PM_NORMAL | BITS_DR_100HZ_74HZ | BIT_XEN | BIT_YEN | BIT_ZEN },

    /* Set full scale and block data update */
    { REG_CTRL_REG4, BITS_FS_100G | BIT_BDU },

    NO_MORE_ENTRIES
};

/**
 * @brief Configures the IMU from scratch.
 *
 * @return whether the configuration succeeded.
 */
static bool configure(void);

static Logger* logger;

namespace teller::drivers::imu {

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
    if (!readRegisterByte(REG_WHO_AM_I, value) || value != 0x32) {
        logger->error("IMU not found");
        return false;
    } else {
        /* Run configuration from scratch */
        return configure();
    }
}

bool update(measurement_3d_t& acceleration, measurement_3d_t& angularVelocity)
{
    std::uint8_t txBuf[7] = { READ_REGISTER_AUTO_INCREMENT(REG_OUT_X_H) };
    std::uint8_t rxBuf[7] = { 0x00 };
    std::uint32_t now;

    /* TODO: timing should be better */
    system::delayMsec(20);

    /* Read accelerometer measurement */
    now = system::getTimeSinceBootMsec();
    if (!spi::transfer(address, txBuf, rxBuf, sizeof(txBuf))) {
        return false;
    }

    /* TODO(ntamas): lock for atomic modification? */
    acceleration.timestampInMsec = now;
    acceleration.value.set(
        static_cast<std::int16_t>((rxBuf[1] << 8) + rxBuf[2]) * ACCEL_SCALE,
        static_cast<std::int16_t>((rxBuf[3] << 8) + rxBuf[4]) * ACCEL_SCALE,
        static_cast<std::int16_t>((rxBuf[5] << 8) + rxBuf[6]) * ACCEL_SCALE);

    // No gyro in this IMU
    angularVelocity.timestampInMsec = now;
    angularVelocity.value.set(0.0f, 0.0f, 0.0f);

    return true;
}
}

static bool readRegisterByte(uint8_t index, uint8_t& result)
{
    uint8_t txBuf[] = { READ_REGISTER(index), 0x00 };
    uint8_t rxBuf[] = { 0x00, 0x00 };
    if (!spi::transfer(address, txBuf, rxBuf, sizeof(txBuf))) {
        return false;
    }
    result = rxBuf[1];
    return true;
}

static bool writeRegisterByte(uint8_t index, uint8_t value, bool verify)
{
    uint8_t buf[] = { WRITE_REGISTER(index), value };
    if (!spi::transfer(address, buf, sizeof(buf))) {
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

    while (entry->reg != 0) {
        /* Write to configuration register */
        if (!writeRegisterByte(entry->reg, entry->value, /* verify = */ true)) {
            logger->error("Setting IMU reg %dfailed", entry->reg);
            return false;
        }

        entry++;
    }

    return true;
}
