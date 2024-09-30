#include "hal/spi.h"
#include "config.h"
#include "core/utils/crc.h"
#include "drivers/sdcard.h"
#include "hal/system.h"
#include "modules/debug.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

#if defined(TELLER_BOARD_NUCLEO144)
static const spi::address_t address = spi::NO_ADDRESS;
#elif defined(TELLER_BOARD_STM32F4)
static const spi::address_t address = { .bus = 1, .device = 0 };
#else
static const spi::address_t address = spi::NO_ADDRESS;
#endif

/** Default timeout to use when sending SD card commands */
static const uint32_t DEFAULT_SD_CARD_TIMEOUT = 250;

/* SD card command set */
#define CMD_GO_IDLE_STATE 0
#define CMD_SEND_IF_COND 8
#define CMD_APP_CMD 55
#define CMD_READ_OCR 58

/* SD card app command set */
#define CMD_APP_SEND_OP_COND 41

/* SD card common responses */
#define RESPONSE_IDLE 0x01
#define RESPONSE_ERASE_RESET 0x02
#define RESPONSE_ILLEGAL_COMMAND 0x04
#define RESPONSE_CRC_ERROR 0x08
#define RESPONSE_ERASE_SEQUENCE_ERROR 0x10
#define RESPONSE_ADDRESS_ERROR 0x20
#define RESPONSE_PARAMETER_ERROR 0x40

static bool tryInitialization(void);
static void prepareCommand(uint8_t* buf, uint8_t cmd, uint32_t arg);
static uint8_t sendCommand(uint8_t cmd, uint32_t arg = 0, uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static bool readR4Response(uint8_t* buf, uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static bool waitUntilIdle(uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);

/** The logger used by the driver */
static Logger* logger;

/** Stores whether the SD card was initialized successfully */
static bool cardFound;

namespace teller::drivers::sdcard {

bool init()
{
    bool success;

    /* TODO: ensure that the clock frequency is between 100 kHz and 400 kHz */
    logger = getLogger(MODULE_ID_EDR);

    /* We cannot call tryInitialization() here because we do not have the
     * facilities of FreeRTOS at our disposal yet. It will be called later
     * in setup()
     */
    cardFound = false;

    success = (logger != nullptr);
    if (!success) {
        destroy();
    }

    return success;
}

void destroy()
{
}

std::unique_ptr<littlefs::FilesystemConfig> createFilesystemConfiguration(void)
{
    return nullptr;
}

bool setup(void)
{
    const char* name = getStorageAreaName(STORAGE_AREA_SD_CARD);

    if (!logger) {
        return false;
    }

    cardFound = tryInitialization();

    if (cardFound) {
        logger->info("%s: found", name);
    } else {
        logger->error("%s: not found", name);
    }

    return true;
}

uint32_t getTotalSize()
{
    return 0;
}

bool readData(uint8_t* buf, uint32_t address, size_t length)
{
    memset(buf, 0, length);
    return true;
}

}

/**
 * @brief Prepares a standard 6-byte SD card command in a buffer.
 *
 * @param buf  the buffer in which the command is prepared
 * @param cmd  the command code between 0 and 63, inclusive
 * @param arg  the argument of the command
 */
static void prepareCommand(uint8_t* buf, uint8_t cmd, uint32_t arg)
{
    buf[0] = (cmd & 0x3F) | 0x40;
    buf[1] = (arg >> 24) & 0xFF;
    buf[2] = (arg >> 16) & 0xFF;
    buf[3] = (arg >> 8) & 0xFF;
    buf[4] = (arg) & 0xFF;
    buf[5] = (crc7_sd(0, buf, 5) << 1) | 0x01;
}

/**
 * @brief Sends a standard 6-byte SD card command and reads the response.
 *
 * @param cmd  the command code between 0 and 63, inclusive
 * @param arg  the argument of the command
 * @param timeout  maximum number of milliseconds to wait for the SD card to
 *        become ready
 * @return the response byte; 0xFF if the command failed
 */
static uint8_t sendCommand(uint8_t cmd, uint32_t arg, uint32_t timeout)
{
    uint8_t buf[10];
    uint8_t i;

    if (!waitUntilIdle(timeout)) {
        return 0xFF;
    }

    for (uint8_t tries = 0; tries < 5; tries++) {
        memset(buf, 0xff, sizeof(buf));
        prepareCommand(buf, cmd, arg);

        /* Response should arrive within 16 clock cycles so it should be enough
         * to send 8 bytes. We send 10 nevertheless and look for the response
         * byte at the end */
        if (!spi::transfer(address, buf, sizeof(buf), spi::NO_CHIP_SELECT)) {
            return 0xff;
        }

        for (i = 6; i < 10; i++) {
            if ((buf[i] & 0x80) == 0) {
                return buf[i];
            }
        }
    }

    return 0xFF;
}

/**
 * @brief Sends a standard 6-byte app command and reads the response.
 *
 * @param cmd  the command code between 0 and 63, inclusive
 * @param arg  the argument of the command
 * @return the response byte; 0xFF if the command failed
 */
static uint8_t sendAppCommand(uint8_t cmd, uint32_t arg)
{
    if (sendCommand(CMD_APP_CMD, 0) != RESPONSE_IDLE) {
        return 0xFF;
    } else {
        return sendCommand(cmd, arg);
    }
}

/**
 * @brief Reads an R4 response.
 *
 * @param buf  the buffer to read the response into. It must be at least 4 bytes long.
 */
static bool readR4Response(uint8_t* buf, uint32_t timeout)
{
    memset(buf, 0xff, 4);
    return spi::transfer(address, buf, 4, spi::NO_CHIP_SELECT);
}

static bool tryInitialization(void)
{
    uint8_t buf[10];
    bool success = false;
    const char* name = getStorageAreaName(STORAGE_AREA_SD_CARD);

    if (!spi::unselect(address)) {
        return false;
    }

    /* Step 1: set MOSI and CS to high and apply 74 or more clock pulses to
     * SCLK */
    memset(buf, 0xff, sizeof(buf));
    if (!spi::transfer(address, buf, sizeof(buf), spi::NO_CHIP_SELECT)) {
        goto cleanup;
    }

    if (!spi::select(address)) {
        goto cleanup;
        return false;
    }

    /* Step 2: send reset command and read response */
    if (sendCommand(CMD_GO_IDLE_STATE) != RESPONSE_IDLE) {
        logger->error("%s: reset failed", name);
        goto cleanup;
    }

    /* Step 3: send CMD8 with argument of 0x1AA. If this is rejected with an
     * illegal command response, the card is SDC v1 or MMC v3 so it is not an
     * SDHC/SDXC card */
    if (sendCommand(CMD_SEND_IF_COND, 0x1aa) != RESPONSE_IDLE) {
        logger->error("%s: not an SD card", name);
        goto cleanup;
    }
    if (!readR4Response(buf) || (buf[2] & 0x01) != 1 || buf[3] != 0xaa) {
        logger->error("%s: not an SD card", name);
        goto cleanup;
    }

    /* Step 4: initiate initialization */
    if (sendAppCommand(CMD_APP_SEND_OP_COND, 0x40000000) != RESPONSE_IDLE) {
        goto cleanup;
    }

    /* Step 5: read OCR register */
    if (sendCommand(CMD_READ_OCR) != RESPONSE_IDLE) {
        goto cleanup;
    }
    if (!readR4Response(buf) || (buf[0] & 0xc0) != 0xc0) {
        goto cleanup;
    }

    /* Success! */
    success = true;

cleanup:
    spi::unselect(address);

    return success;
}

static bool waitUntilIdle(uint32_t timeout)
{
    uint8_t busy;
    uint32_t now = system::getTimeSinceBootMsec();
    uint32_t deadline = (timeout < std::numeric_limits<uint32_t>::max())
        ? now + timeout
        : std::numeric_limits<uint32_t>::max();

    do {
        busy = 0xff;
        if (!spi::transfer(address, &busy, sizeof(busy), spi::NO_CHIP_SELECT)) {
            return false;
        }
        now = system::getTimeSinceBootMsec();
    } while (busy != 0xff && now < deadline);

    return busy == 0xff;
}
