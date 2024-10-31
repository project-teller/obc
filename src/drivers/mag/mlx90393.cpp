#include <cstring>
#include <limits>

#include "config.h"
#include "drivers/mag.h"
#include "hal/spi.h"
#include "hal/system.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

#if defined(TELLER_BOARD_NUCLEO144)
static const spi::address_t address = spi::NO_ADDRESS;
#elif defined(TELLER_BOARD_STM32F4)
/* SPI bus 3, CS pin 1 */
static const spi::address_t address = { .bus = 2, .device = 0 };
#else
static const spi::address_t address = spi::NO_ADDRESS;
#endif

#define CMD_NOP 0x00
#define CMD_START_BURST_MODE 0x10
#define CMD_START_WAKE_UP_ON_CHANGE_MODE 0x20
#define CMD_START_SINGLE_MEASUREMENT_MODE 0x30
#define CMD_READ_MEASUREMENT 0x40
#define CMD_READ_REGISTER 0x50
#define CMD_WRITE_REGISTER 0x60
#define CMD_EXIT_MODE 0x80
#define CMD_RESET 0xF0

#define MEASURE_Z 0x08
#define MEASURE_Y 0x04
#define MEASURE_X 0x02
#define MEASURE_TEMPERATURE 0x01
#define MEASURE_XYZ (MEASURE_X | MEASURE_Y | MEASURE_Z)
#define MEASURE_ALL (MEASURE_XYZ | MEASURE_TEMPERATURE)

#define REG_CONF1 0x00
#define REG_CONF2 0x01
#define REG_CONF3 0x02
#define REG_CONF4 0x03

#define REG_MASK_BURST_DATA_RATE 0x003F
#define REG_MASK_BURST_DATA_RATE_SHIFT 0
#define REG_MASK_BURST_SEL 0x03C0
#define REG_MASK_BURST_SEL_SHIFT 6
#define REG_MASK_GAIN 0x0070
#define REG_MASK_GAIN_SHIFT 4
#define REG_MASK_COMM_MODE 0x6000
#define REG_MASK_COMM_MODE_SHIFT 13
#define REG_MASK_FILTER 0x001C
#define REG_MASK_FILTER_SHIFT 2
#define REG_MASK_OVERSAMPLING_RATE 0x0003
#define REG_MASK_OVERSAMPLING_RATE_SHIFT 0
#define REG_MASK_RESOLUTION_X 0x0060
#define REG_MASK_RESOLUTION_X_SHIFT 5
#define REG_MASK_RESOLUTION_Y 0x0180
#define REG_MASK_RESOLUTION_Y_SHIFT 7
#define REG_MASK_RESOLUTION_Z 0x0600
#define REG_MASK_RESOLUTION_Z_SHIFT 9
#define REG_MASK_TEMPERATURE_OVERSAMPLING_RATE 0x1800
#define REG_MASK_TEMPERATURE_OVERSAMPLING_RATE_SHIFT 11

#define STATUS_BURST_MODE (1 << 7)
#define STATUS_WOC_MODE (1 << 6)
#define STATUS_SM_MODE (1 << 5)
#define STATUS_ERROR (1 << 4)
#define STATUS_SED (1 << 3)
#define STATUS_RESET (1 << 2)

#define IS_ERROR(response) ((response & STATUS_ERROR) != 0)

#define GAIN_SETTING GAIN_1X
#define RESOLUTION_SETTING RESOLUTION_16

typedef enum {
    AXIS_X,
    AXIS_Y,
    AXIS_Z,
} axis_t;

typedef enum {
    FILTER_0,
    FILTER_1,
    FILTER_2,
    FILTER_3,
    FILTER_4,
    FILTER_5,
    FILTER_6,
    FILTER_7,
} filter_t;

typedef enum {
    GAIN_5X,
    GAIN_4X,
    GAIN_3X,
    GAIN_2_5X,
    GAIN_2X,
    GAIN_1_67X,
    GAIN_1_33X,
    GAIN_1X,
} gain_t;

typedef enum {
    OVERSAMPLING_RATE_0,
    OVERSAMPLING_RATE_1,
    OVERSAMPLING_RATE_2,
    OVERSAMPLING_RATE_3,
} oversampling_rate_t;

typedef enum {
    RESOLUTION_16,
    RESOLUTION_17,
    RESOLUTION_18,
    RESOLUTION_19,
} resolution_t;

typedef struct {
    uint16_t temperature;
    int16_t x;
    int16_t y;
    int16_t z;
} raw_measurement_t;

/**
 * @brief Lookup table to convert raw values to uT based on [HALLCONF][GAIN_SEL][RES].
 */
const float mlx90393_lsb_lookup[2][8][4][2] = {

    /* HALLCONF = 0xC (default) */
    {
        /* GAIN_SEL = 0, 5x gain */
        { { 0.751, 1.210 }, { 1.502, 2.420 }, { 3.004, 4.840 }, { 6.009, 9.680 } },
        /* GAIN_SEL = 1, 4x gain */
        { { 0.601, 0.968 }, { 1.202, 1.936 }, { 2.403, 3.872 }, { 4.840, 7.744 } },
        /* GAIN_SEL = 2, 3x gain */
        { { 0.451, 0.726 }, { 0.901, 1.452 }, { 1.803, 2.904 }, { 3.605, 5.808 } },
        /* GAIN_SEL = 3, 2.5x gain */
        { { 0.376, 0.605 }, { 0.751, 1.210 }, { 1.502, 2.420 }, { 3.004, 4.840 } },
        /* GAIN_SEL = 4, 2x gain */
        { { 0.300, 0.484 }, { 0.601, 0.968 }, { 1.202, 1.936 }, { 2.403, 3.872 } },
        /* GAIN_SEL = 5, 1.667x gain */
        { { 0.250, 0.403 }, { 0.501, 0.807 }, { 1.001, 1.613 }, { 2.003, 3.227 } },
        /* GAIN_SEL = 6, 1.333x gain */
        { { 0.200, 0.323 }, { 0.401, 0.645 }, { 0.801, 1.291 }, { 1.602, 2.581 } },
        /* GAIN_SEL = 7, 1x gain */
        { { 0.150, 0.242 }, { 0.300, 0.484 }, { 0.601, 0.968 }, { 1.202, 1.936 } },
    },

    /* HALLCONF = 0x0 */
    {
        /* GAIN_SEL = 0, 5x gain */
        { { 0.787, 1.267 }, { 1.573, 2.534 }, { 3.146, 5.068 }, { 6.292, 10.137 } },
        /* GAIN_SEL = 1, 4x gain */
        { { 0.629, 1.014 }, { 1.258, 2.027 }, { 2.517, 4.055 }, { 5.034, 8.109 } },
        /* GAIN_SEL = 2, 3x gain */
        { { 0.472, 0.760 }, { 0.944, 1.521 }, { 1.888, 3.041 }, { 3.775, 6.082 } },
        /* GAIN_SEL = 3, 2.5x gain */
        { { 0.393, 0.634 }, { 0.787, 1.267 }, { 1.573, 2.534 }, { 3.146, 5.068 } },
        /* GAIN_SEL = 4, 2x gain */
        { { 0.315, 0.507 }, { 0.629, 1.014 }, { 1.258, 2.027 }, { 2.517, 4.055 } },
        /* GAIN_SEL = 5, 1.667x gain */
        { { 0.262, 0.422 }, { 0.524, 0.845 }, { 1.049, 1.689 }, { 2.097, 3.379 } },
        /* GAIN_SEL = 6, 1.333x gain */
        { { 0.210, 0.338 }, { 0.419, 0.676 }, { 0.839, 1.352 }, { 1.678, 2.703 } },
        /* GAIN_SEL = 7, 1x gain */
        { { 0.157, 0.253 }, { 0.315, 0.507 }, { 0.629, 1.014 }, { 1.258, 2.027 } },
    }
};

/**
 * @brief Lookup table for conversion time based on [DIF_FILT][OSR].
 */
const float mlx90393_tconv[8][4] = {
    /* DIG_FILT = 0 */
    { 1.27, 1.84, 3.00, 5.30 },
    /* DIG_FILT = 1 */
    { 1.46, 2.23, 3.76, 6.84 },
    /* DIG_FILT = 2 */
    { 1.84, 3.00, 5.30, 9.91 },
    /* DIG_FILT = 3 */
    { 2.61, 4.53, 8.37, 16.05 },
    /* DIG_FILT = 4 */
    { 4.15, 7.60, 14.52, 28.34 },
    /* DIG_FILT = 5 */
    { 7.22, 13.75, 26.80, 52.92 },
    /* DIG_FILT = 6 */
    { 13.36, 26.04, 51.38, 102.07 },
    /* DIG_FILT = 7 */
    { 25.65, 50.61, 100.53, 200.37 },
};

static bool configure(void);
static bool sendCommand(uint8_t command, uint8_t& response);
static bool readMeasurement(raw_measurement_t& measurement);
static bool readRegister(uint8_t index, uint16_t& value);
static bool updateRegister(uint8_t index, uint16_t value, uint16_t mask, bool verify = true);
static bool writeRegister(uint8_t index, uint16_t value, bool verify = true);

static bool setBurstIntervalMsec(uint16_t value);
static bool setBurstMeasurementConfiguration(uint8_t value);
static bool setFilter(filter_t filter);
static bool setGain(gain_t gain);
static bool setOversamplingRate(oversampling_rate_t rate);
static bool setResolution(axis_t axis, resolution_t resolution);
static bool setTemperatureOversamplingRate(oversampling_rate_t rate);

static Logger* logger;
static raw_measurement_t rawMeasurement;

namespace teller::drivers::mag {

bool init()
{
    /* Most of the initialization is done in setup() because we need to run
     * SPI transfers with interrupts */
    logger = getLogger(MODULE_ID_MAG);
    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
}

bool setup()
{
    return configure();
}

bool update(measurement_3d_t& magneticVector, float& temperature)
{
    uint8_t tries = 20;
    bool success = false;

    /* Read measurement */
    while (tries > 0) {
        if (readMeasurement(rawMeasurement)) {
            success = true;
            break;
        }

        tries--;
        system::delayMsec(10);
    }

    if (success) {
        /* TODO(ntamas): lock for atomic modification? */
        magneticVector.timestampInMsec = system::getTimeSinceBootMsec();

        /* mlx90393_lsb_lookup gives us microteslas. We need milligauss, hence
         * the extra multiplier. */
        magneticVector.value.set(
            rawMeasurement.x * mlx90393_lsb_lookup[0][GAIN_SETTING][RESOLUTION_SETTING][0] * 10.0f,
            rawMeasurement.y * mlx90393_lsb_lookup[0][GAIN_SETTING][RESOLUTION_SETTING][0] * 10.0f,
            rawMeasurement.z * mlx90393_lsb_lookup[0][GAIN_SETTING][RESOLUTION_SETTING][1] * 10.0f);

        /* Offset and divisor for the temperature is in Table 6 of the datasheet */
        temperature = (rawMeasurement.temperature - 45114) / 45.2f;
    }

    return success;
}
}

/**
 * @brief Configures the magnetometer from scratch.
 *
 * @return whether the configuration succeeded.
 */
static bool configure()
{
    uint8_t response;

    /* This procedure is based on the Adafruit MLX90393 driver, see
     * https://github.com/adafruit/Adafruit_MLX90393_Library
     */

    /* Exit the current mode (if any) */
    if (!sendCommand(CMD_EXIT_MODE, response) || IS_ERROR(response)) {
        return false;
    }

    /* 1ms wait is advised by the datasheet */
    teller::hal::system::delayMsec(1);

    /* Reset command does not generate a response code */
    if (!sendCommand(CMD_RESET, response)) {
        return false;
    }

    /* Power-on reset delay is 1.6 msec at most so 2 should be OK */
    teller::hal::system::delayMsec(2);

    /* Set communication mode to SPI, set gain to 1x, set resolution to 16,
     * set oversampling rate to 2, set digital filtering to 6.
     *
     * Note that the oversampling rate and the digital filtering setting affects
     * the maximum data rate that can be achieved in burst mode. Refer to
     * Table 17 of the datasheet. The current settings are suitable for 17.7
     * measurements per second, which is more than the target rate of 10 Hz.
     */
    if (
        /* clang-format off */
        !updateRegister(REG_CONF2, 0b10 << REG_MASK_COMM_MODE_SHIFT, REG_MASK_COMM_MODE) ||
        !setGain(GAIN_SETTING) ||
        !setResolution(AXIS_X, RESOLUTION_SETTING) ||
        !setResolution(AXIS_Y, RESOLUTION_SETTING) ||
        !setResolution(AXIS_Z, RESOLUTION_SETTING) ||
        !setOversamplingRate(OVERSAMPLING_RATE_2) ||
        !setTemperatureOversamplingRate(OVERSAMPLING_RATE_2) ||
        !setFilter(FILTER_6) ||
        !setBurstIntervalMsec(20) ||
        !setBurstMeasurementConfiguration(MEASURE_ALL)
        /* clang-format on */
    ) {
        return false;
    }

    /* Start burst measurement mode */
    if (!sendCommand(CMD_START_BURST_MODE, response)) {
        return false;
    }
    if (IS_ERROR(response) || !(response & STATUS_BURST_MODE)) {
        return false;
    }

    return true;
}

/**
 * @brief Sends a command to the magnetometer.
 *
 * @return the status byte returned from the command
 */
static bool sendCommand(uint8_t command, uint8_t& response)
{
    uint8_t txBuf[2] = { command, 0x00 };
    uint8_t rxBuf[2] = { 0x00, 0x00 };

    if (!spi::transfer(address, txBuf, rxBuf, sizeof(txBuf))) {
        return false;
    }

    /* Reset commands do not generate a response byte */
    response = command != CMD_RESET ? rxBuf[1] : 0x00;
    return !IS_ERROR(response);
}

/**
 * @brief Reads a measurement from the magnetometer.
 *
 * @return whether the measurement was successful
 */
static bool readMeasurement(raw_measurement_t& measurement)
{
    uint8_t txBuf[10] = { CMD_READ_MEASUREMENT | MEASURE_ALL, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t rxBuf[10] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    if (!spi::transfer(address, txBuf, rxBuf, sizeof(txBuf))) {
        return false;
    }

    if (IS_ERROR(rxBuf[1])) {
        return false;
    }

    measurement.temperature = (rxBuf[2] << 8) | rxBuf[3];
    measurement.x = (rxBuf[4] << 8) | rxBuf[5];
    measurement.y = (rxBuf[6] << 8) | rxBuf[7];
    measurement.z = (rxBuf[8] << 8) | rxBuf[9];

    if (RESOLUTION_SETTING == RESOLUTION_18) {
        measurement.x -= 0x8000;
        measurement.y -= 0x8000;
        measurement.z -= 0x8000;
    }
    if (RESOLUTION_SETTING == RESOLUTION_19) {
        measurement.x -= 0x4000;
        measurement.y -= 0x4000;
        measurement.z -= 0x4000;
    }

    return true;
}

/**
 * @brief Reads the value in the register with the given index.
 *
 * @param index  the index of the register to read
 * @param value  the value is returned here
 * @return whether the read was successful
 */
static bool readRegister(uint8_t index, uint16_t& value)
{
    uint8_t txBuf[5] = { CMD_READ_REGISTER, 0x00, 0x00, 0x00, 0x00 };
    uint8_t rxBuf[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

    txBuf[1] = index << 2;

    if (!spi::transfer(address, txBuf, rxBuf, sizeof(txBuf))) {
        return false;
    }

    if (IS_ERROR(rxBuf[2])) {
        return false;
    }

    value = (rxBuf[3] << 8) | rxBuf[4];
    return true;
}

/**
 * @brief Writes the given value in the register with the given index.
 *
 * @param index  the index of the register to write
 * @param value  the value to write
 * @param verify whether the value of the register should be read back for
 *        verification
 * @return whether the write was successful
 */
static bool writeRegister(uint8_t index, uint16_t value, bool verify)
{
    uint8_t txBuf[5] = { CMD_WRITE_REGISTER, 0x00, 0x00, 0x00, 0x00 };
    uint8_t rxBuf[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

    txBuf[1] = value >> 8;
    txBuf[2] = value & 0xFF;
    txBuf[3] = index << 2;

    if (!spi::transfer(address, txBuf, rxBuf, sizeof(txBuf))) {
        return false;
    }

    if (IS_ERROR(rxBuf[4])) {
        return false;
    }

    if (verify) {
        uint16_t observedValue;
        return readRegister(index, observedValue) && observedValue == value;
    } else {
        return true;
    }
}

/**
 * @brief Updates part of the value in the register with the given index.
 *
 * @param index  the index of the register to write
 * @param value  the value to write
 * @param mask   the mask of the value. The bits in the register where the mask
 *        is 0 will be kept; only those bits will be updated where the mask is 1.
 * @param verify whether the value of the register should be read back for
 *        verification
 */
static bool updateRegister(uint8_t index, uint16_t value, uint16_t mask, bool verify)
{
    uint16_t originalValue;

    if (!mask) {
        return true;
    }

    if (!readRegister(index, originalValue)) {
        return false;
    }

    value = (originalValue & ~mask) | (value & mask);

    return writeRegister(index, value, verify);
}

/**
 * @brief Sets the time interval between measurements in burst mode, in msec.
 *
 * Note that the value will be rounded to the nearest 20 msec. The interval
 * refers to the time between the end of a measurement and the start of the
 * next measurement.
 *
 * @param value  the time interval between measurements in burst mode, in
 *        milliseconds.
 */
static bool setBurstIntervalMsec(uint16_t value)
{
    uint8_t dataRateRegValue;

    value = (value < 20) ? 20 : ((value > 1260) ? 1260 : value);
    dataRateRegValue = (value + 10) / 20;

    return updateRegister(
        REG_CONF2,
        dataRateRegValue << REG_MASK_BURST_DATA_RATE_SHIFT,
        REG_MASK_BURST_DATA_RATE);
}

/**
 * @brief Sets the measurements to perform in burst mode.
 *
 * @param value  the measurements to perform; see the \c MEASURE_... macros
 */
static bool setBurstMeasurementConfiguration(uint8_t value)
{
    return updateRegister(
        REG_CONF2,
        value << REG_MASK_BURST_SEL_SHIFT,
        REG_MASK_BURST_SEL);
}

/**
 * @brief Sets the digital filter setting of the sensor.
 */
static bool setFilter(filter_t filter)
{
    return updateRegister(REG_CONF3, filter << REG_MASK_FILTER_SHIFT, REG_MASK_FILTER);
}

/**
 * @brief Sets the sensor gain to the specified value.
 */
static bool setGain(gain_t gain)
{
    return updateRegister(REG_CONF1, gain << REG_MASK_GAIN_SHIFT, REG_MASK_GAIN);
}

/**
 * @brief Sets the resolution of the sensor along one of the axes.
 */
static bool setResolution(axis_t axis, resolution_t resolution)
{
    uint16_t value;
    uint16_t mask;

    switch (axis) {
    case AXIS_X:
        mask = REG_MASK_RESOLUTION_X;
        value = resolution << REG_MASK_RESOLUTION_X_SHIFT;
        break;

    case AXIS_Y:
        mask = REG_MASK_RESOLUTION_Y;
        value = resolution << REG_MASK_RESOLUTION_Y_SHIFT;
        break;

    case AXIS_Z:
        mask = REG_MASK_RESOLUTION_Z;
        value = resolution << REG_MASK_RESOLUTION_Z_SHIFT;
        break;

    default:
        value = 0;
        mask = 0;
        break;
    }

    return updateRegister(REG_CONF3, value, mask);
}

/**
 * @brief Sets the oversampling rate of the magnetic vector measurements of the sensor.
 */
static bool setOversamplingRate(oversampling_rate_t rate)
{
    return updateRegister(
        REG_CONF3, rate << REG_MASK_OVERSAMPLING_RATE_SHIFT,
        REG_MASK_OVERSAMPLING_RATE);
}

/**
 * @brief Sets the oversampling rate of the temperature measurements of the sensor.
 */
static bool setTemperatureOversamplingRate(oversampling_rate_t rate)
{
    return updateRegister(
        REG_CONF3, rate << REG_MASK_TEMPERATURE_OVERSAMPLING_RATE_SHIFT,
        REG_MASK_TEMPERATURE_OVERSAMPLING_RATE);
}
