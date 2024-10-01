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
#define RESPONSE_OK 0x00
#define RESPONSE_IDLE 0x01
#define RESPONSE_ERASE_RESET 0x02
#define RESPONSE_ILLEGAL_COMMAND 0x04
#define RESPONSE_CRC_ERROR 0x08
#define RESPONSE_ERASE_SEQUENCE_ERROR 0x10
#define RESPONSE_ADDRESS_ERROR 0x20
#define RESPONSE_PARAMETER_ERROR 0x40

static bool tryInitialization(void);
static void prepareCommand(uint8_t* buf, uint8_t cmd, uint32_t arg);
static uint8_t sendCommand(
    uint8_t cmd, uint32_t arg = 0, uint8_t* buf = nullptr, uint8_t size = 0,
    uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
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
 * @param buf  buffer in which to place the part of the response after the
 *        initial response byte. null for R1 responses that have no additional
 *        content
 * @param size size of the buffer, i.e. the expected number of additional bytes
 *        after the first response byte
 * @param timeout  maximum number of milliseconds to wait for the SD card to
 *        become ready
 * @return the response byte; 0xFF if the command failed
 */
static uint8_t sendCommand(uint8_t cmd, uint32_t arg, uint8_t* buf, uint8_t size, uint32_t timeout)
{
    uint8_t txBuf[10];
    uint8_t rxBuf[10];
    uint8_t i;
    uint8_t response = 0xff;

    if (size > sizeof(rxBuf)) {
        size = sizeof(rxBuf);
    }

    if (!waitUntilIdle(timeout)) {
        goto exit;
    }

    for (uint8_t tries = 0; tries < 5; tries++) {
        memset(txBuf, 0xff, sizeof(txBuf));
        prepareCommand(txBuf, cmd, arg);

        /* Send the command -- 6 bytes long */
        if (!spi::transfer(address, txBuf, rxBuf, 6, spi::NO_CHIP_SELECT)) {
            goto exit;
        }

        /* Response should arrive within 16 clock cycles so it is enough
         * to wait for 2 more bytes, but let's be on the safe side and wait for
         * at most 4 bytes. We send the bytes one by one, reading one byte in
         * the response until we get a byte where the MSB is zero -- this will
         * be the response code */
        for (i = 0; i < 4; i++) {
            txBuf[0] = 0xff;
            if (!spi::transfer(address, txBuf, rxBuf, 1, spi::NO_CHIP_SELECT)) {
                goto exit;
            }
            if ((rxBuf[0] & 0x80) == 0) {
                response = rxBuf[0];
                goto readRest;
            }
        }
    }

readRest:
    if (buf != nullptr && size > 0) {
        memset(txBuf, 0xff, size);
        if (!spi::transfer(address, txBuf, buf, size, spi::NO_CHIP_SELECT)) {
            response = 0xff;
            goto exit;
        }
    }

exit:
    return response;
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
     * SDHC/SDXC card. Expected response is an R4 response.
     *
     * In 0x1aa, the 0x1.. part checks whether the voltage of 2.7-3.6V is
     * accepted. The 0xaa part is a random check pattern. The check pattern
     * will be sent back as-is. The byte in front of it will be set in a way
     * that indicates the supported voltages; here we need to check the presence
     * of the bit we have sent.
     *
     * Source: https://luckyresistor.me/cat-protector/software/sdcard-2/
     */
    if (sendCommand(CMD_SEND_IF_COND, 0x1aa, buf, 4) != RESPONSE_IDLE) {
        logger->error("%s: not an SD card", name);
        goto cleanup;
    }
    if ((buf[2] & 0x01) != 1 || buf[3] != 0xaa) {
        logger->error("%s: not an SD card", name);
        goto cleanup;
    }

    /* Step 4: initiate initialization, setting the HCS flag, which corresponds
     * to fast transfer rates. */
    for (uint8_t i = 0; i < 10; i++) {
        uint8_t response = sendAppCommand(CMD_APP_SEND_OP_COND, 0x40000000);
        if (response == RESPONSE_IDLE) {
            /* this is okay, repeat the command */
        } else if (response == RESPONSE_OK) {
            /* this is what we were waiting for; see:
             * https://electronics.stackexchange.com/a/238217
             */
            break;
        } else {
            /* unexpected response, bail out */
            goto cleanup;
        }
    }

    /* Step 5: read OCR register. We are expecting an R3 response, which is
     * 4 bytes long. During initialization, bit 31 of the OCR register
     * (Card Power Up Status) must be 1. At that point we can check bit 30,
     * which should also be 1 for an SDHC/SDXC card. */
    if (sendCommand(CMD_READ_OCR, 0x00, buf, 4) != RESPONSE_OK) {
        goto cleanup;
    }
    if ((buf[0] & 0xc0) != 0xc0) {
        logger->error("%s: not an SDHC/SDXC card", name);
        goto cleanup;
    }

    /* Success! */
    /* TODO: bump the clock of the SPI bus to 32 MHz */
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
