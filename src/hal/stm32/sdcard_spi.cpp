#include "config.h"
#include "core/utils/crc.h"
#include "hal/sdcard.h"
#include "hal/spi.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

/*
static const spi::address_t address = {
    .bus = 0,
    .device = 0
};
*/
static const spi::address_t address = spi::NO_ADDRESS;

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
static uint8_t sendCommand(uint8_t cmd, uint32_t arg = 0);
static bool readR4Response(uint8_t* buf);
static bool waitUntilIdle(void);

/** The logger used by the driver */
static Logger* logger;

/** Stores whether the SD card was initialized successfully */
static bool cardFound;

namespace teller::hal::sdcard {

bool init()
{
    bool success;

    /* TODO: ensure that the clock frequency is between 100 kHz and 400 kHz */
    logger = getLogger(MODULE_ID_EDR);
    cardFound = tryInitialization();

    /* cardFound == false is okay, we still want to report that we did not find
     * the card in setup() */
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
    const char* name = getStorageAreaName(STORAGE_AREA_FLASH_MEMORY);

    if (!logger) {
        return false;
    }

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
 * @return the response byte; 0xFF if the command failed
 */
static uint8_t sendCommand(uint8_t cmd, uint32_t arg)
{
    uint8_t buf[10];
    uint8_t i;

    if (!waitUntilIdle()) {
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
static bool readR4Response(uint8_t* buf)
{
    memset(buf, 0xff, 4);
    return spi::transfer(address, buf, 4, spi::NO_CHIP_SELECT);
}

static bool tryInitialization(void)
{
    uint8_t buf[10];
    bool success = false;

    spi::select(address);

    /* Step 1: set MOSI and CS to high and apply 74 or more clock pulses to
     * SCLK */
    memset(buf, 0xff, sizeof(buf));
    if (!spi::transfer(address, buf, sizeof(buf), spi::NO_CHIP_SELECT)) {
        goto cleanup;
    }

    /* Step 2: send reset command and read response */
    if (sendCommand(CMD_GO_IDLE_STATE) != RESPONSE_IDLE) {
        goto cleanup;
    }

    /* Step 3: send CMD8 with argument of 0x1AA. If this is rejected with an
     * illegal command response, the card is SDC v1 or MMC v3 so it is not an
     * SDHC/SDXC card */
    if (sendCommand(CMD_SEND_IF_COND, 0x1aa) != RESPONSE_IDLE) {
        goto cleanup;
    }
    if (!readR4Response(buf) || (buf[2] & 0x01) != 1 || buf[3] != 0xaa) {
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

static bool waitUntilIdle()
{
    uint8_t busy;

    do {
        busy = 0xFF;
        if (!spi::transfer(address, &busy, sizeof(busy))) {
            return false;
        }
    } while (busy != 0xff);

    return true;
}
