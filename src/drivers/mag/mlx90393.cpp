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

#define STATUS_BURST_MODE (1 << 7)
#define STATUS_WOC_MODE (1 << 6)
#define STATUS_SM_MODE (1 << 5)
#define STATUS_ERROR (1 << 4)
#define STATUS_SED (1 << 3)
#define STATUS_RESET (1 << 2)

#define IS_ERROR(response) ((response & STATUS_ERROR) != 0)

static bool configure(void);
static bool sendCommand(uint8_t command, uint8_t& response);
static bool readRegister(uint8_t index, uint16_t& value);
static bool writeRegister(uint8_t index, uint16_t value, bool verify = true);

static Logger* logger;

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

bool update(measurement_3d_t& magneticVector)
{
    uint32_t now;

    /* TODO: timing should be better */
    system::delayMsec(20);

    /* Read measurement */
    now = system::getTimeSinceBootMsec();

    /* TODO(ntamas): lock for atomic modification? */
    magneticVector.timestampInMsec = now;
    magneticVector.value.set(0, 0, 0);

    return true;
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
    uint16_t regValue;

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

    /* Set communication mode to SPI.
     * We need to set the COMM_MODE[1:0] bits to 10 in register 0x01 */
    if (!readRegister(0x01, regValue)) {
        return false;
    }
    regValue = (regValue & 0b1001111111111111) | (0b10 << 13);
    if (!writeRegister(0x01, regValue)) {
        return false;
    }

    /* TODO: set gain 1x */
    /* TODO: set resolution to 16 */
    /* TODO: set oversampling to 3 */
    /* TODO: set digital filtering to 7 */

    /* Start single measurement mode */
    if (!sendCommand(CMD_START_SINGLE_MEASUREMENT_MODE, response)) {
        return false;
    }
    if (IS_ERROR(response) || !(response & STATUS_SM_MODE)) {
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
