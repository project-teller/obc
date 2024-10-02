#include "hal/spi.h"
#include "config.h"
#include "core/utils/crc.h"
#include "drivers/sdcard.h"
#include "hal/system.h"
#include "modules/debug.h"
#include "modules/log.h"

#include "stm32_hal.h"

using namespace littlefs;
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

/**
 * Default timeout to use when sending SD card commands.
 * This is according to section 4.6.2.1 of the physical spec.
 */
static const uint32_t DEFAULT_SD_CARD_TIMEOUT = 100;

/* Constants for size conversions */
#define BLOCK_SIZE 512

/* SD card command set */
#define CMD_GO_IDLE_STATE 0
#define CMD_SEND_IF_COND 8
#define CMD_SEND_CSD 9
#define CMD_READ_SINGLE_BLOCK 17
#define CMD_READ_MULTIPLE_BLOCK 18
#define CMD_WRITE_SINGLE_BLOCK 24
#define CMD_WRITE_MULTIPLE_BLOCK 25
#define CMD_ERASE_WR_BLK_START_ADDR 32
#define CMD_ERASE_WR_BLK_END_ADDR 33
#define CMD_ERASE 38
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

/* Data tokens */
#define DATA_TOKEN 0xfe

static uint32_t tryInitialization(void);
static bool eraseBlock(uint32_t block);
static void prepareCommand(uint8_t* buf, uint8_t cmd, uint32_t arg);
static bool programBlockFromBuffer(uint32_t block, const uint8_t* buf);
static bool readBlockIntoBuffer(uint32_t block, uint8_t* buf);
static bool readResponseData(
    uint8_t* buf, size_t size, uint8_t token = DATA_TOKEN,
    uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static uint8_t sendCommand(
    uint8_t cmd, uint32_t arg = 0, uint8_t* buf = nullptr, uint8_t size = 0,
    uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static bool waitForDataToken(uint8_t token = DATA_TOKEN,
    uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static bool waitWhileBusy(uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);

/* LittleFS handler functions */
static int sdcard_spi_read(
    const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off,
    void* buffer, lfs_size_t size);
static int sdcard_spi_write(
    const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off,
    const void* buffer, lfs_size_t size);
static int sdcard_spi_erase(const struct lfs_config* cfg, lfs_block_t block);
static int sdcard_spi_sync(const struct lfs_config* cfg);

/** The logger used by the driver */
static Logger* logger;

/** Stores the number of blocks on the SD card. Zero if no card was found. */
static uint32_t blockCount;

namespace teller::drivers::sdcard {

bool init()
{
    bool success;

    /* TODO: ensure that the clock frequency is between 100 kHz and 400 kHz */
    logger = getLogger(MODULE_ID_EDR);
    blockCount = tryInitialization();

    /* blockCount == 0 is okay, we still want to report that we did not find the
     * SD card in setup() */
    success = (logger != nullptr);

    if (!success) {
        destroy();
    }

    return success;
}

void destroy()
{
    blockCount = 0;
}

std::unique_ptr<FilesystemConfig> createFilesystemConfiguration(void)
{
    if (blockCount <= 0) {
        return nullptr;
    }

    auto new_config = std::make_unique<FilesystemConfig>(
        sdcard_spi_read, /* read */
        sdcard_spi_write, /* prog */
        sdcard_spi_erase, /* erase */
        sdcard_spi_sync, /* sync */
        /* read_size = */ BLOCK_SIZE,
        /* prog_size = */ BLOCK_SIZE,
        /* erase_size = */ BLOCK_SIZE,
        /* erase_count = */ blockCount,
        /* block_cycles = */ 500,
        /* cache_size = */ BLOCK_SIZE,
        /* lookahead_size = */ BLOCK_SIZE);

    return new_config;
}

bool setup(void)
{
    const char* name = getStorageAreaName(STORAGE_AREA_SD_CARD);

    if (!logger) {
        return false;
    }

    blockCount = tryInitialization();

    if (blockCount > 0) {
        logger->info("%s: found (%d MB)", name, (blockCount >> 11));
    } else {
        logger->error("%s: not found", name);
    }

    return true;
}

uint32_t getTotalSize()
{
    return blockCount * BLOCK_SIZE;
}

bool readData(uint8_t* buf, uint32_t address, size_t length)
{
    /* TODO: address is byte-aligned, but we can only read entire blocks */
    return false;
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

    if (!waitWhileBusy(timeout)) {
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

static uint32_t tryInitialization(void)
{
    uint8_t buf[16]; /* 16 bytes are needed for the CSD register */
    const char* name = getStorageAreaName(STORAGE_AREA_SD_CARD);

    blockCount = 0;

    if (!spi::unselect(address)) {
        return 0;
    }

    /* Step 1: set MOSI and CS to high and apply 74 or more clock pulses to
     * SCLK */
    memset(buf, 0xff, sizeof(buf));
    if (!spi::transfer(address, buf, 10, spi::NO_CHIP_SELECT)) {
        goto cleanup;
    }

    if (!spi::select(address)) {
        goto cleanup;
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

    /* Step 6: read CSD register to determine the size of the card */
    if (sendCommand(CMD_SEND_CSD, 0x00) != RESPONSE_OK) {
        goto cleanup;
    }
    if (!readResponseData(buf, 16)) {
        goto cleanup;
    }

    /* Parse card size from CSD register */
    if ((buf[0] & 0xC0) != 0x40) {
        /* Not a CSD register value; it must start with 0b01xxxxxx */
        goto cleanup;
    }

    /* TODO: validate CRC at the end of the packet */

    /* Success! */
    blockCount = buf[7] & 0x3F; // two bits are reserved
    blockCount = (blockCount << 8) | buf[8];
    blockCount = (blockCount << 8) | buf[9];
    blockCount = (blockCount + 1) << 10; /* assert BLOCK_SIZE == (1 << 9) */

cleanup:
    spi::unselect(address);

    return blockCount;
}

/**
 * @brief Reads bytes from the device until the given data token is received.
 *
 * @param token  the data token to expect
 * @param timeout  timeout for the operation in milliseconds
 * @return whether the data token was received successfully
 */
static bool waitForDataToken(uint8_t token, uint32_t timeout)
{
    /* Do not use system::getTimeSinceBootMsec() here -- this function is
     * called during initialization when the FreeRTOS facilities are not
     * available yet */
    uint8_t data;
    uint32_t now = HAL_GetTick();
    uint32_t deadline = (timeout < std::numeric_limits<uint32_t>::max())
        ? now + timeout
        : std::numeric_limits<uint32_t>::max();

    do {
        data = 0xff;
        if (!spi::transfer(address, &data, sizeof(data), spi::NO_CHIP_SELECT)) {
            return false;
        }
        now = HAL_GetTick();
    } while (data == 0xff && now < deadline);

    return data == token;
}

/**
 * @brief Reads bytes from the device until the device indicates that it is
 *        not busy any more.
 *
 * @param timeout  timeout for the operation in milliseconds
 * @return whether the device is ready for a new command
 */
static bool waitWhileBusy(uint32_t timeout)
{
    /* Do not use system::getTimeSinceBootMsec() here -- this function is
     * called during initialization when the FreeRTOS facilities are not
     * available yet */
    uint8_t busy;
    uint32_t now = HAL_GetTick();
    uint32_t deadline = (timeout < std::numeric_limits<uint32_t>::max())
        ? now + timeout
        : std::numeric_limits<uint32_t>::max();

    do {
        busy = 0xff;
        if (!spi::transfer(address, &busy, sizeof(busy), spi::NO_CHIP_SELECT)) {
            return false;
        }
        now = HAL_GetTick();
    } while (busy != 0xff && now < deadline);

    return busy == 0xff;
}

/**
 * @brief Reads a block of bytes from the device, expecting a data token in
 *        front of the block and a 16-bit CRC at the end.
 *
 * Also validates the CRC at the end of the block.
 *
 * @param buf      the buffer to read the block into
 * @param size     expected size of the block to read
 * @param token    expected data token in front of the data block
 * @param timeout  timeout for the operation in milliseconds
 * @return whether the read was successful
 */
static bool readResponseData(uint8_t* buf, size_t size, uint8_t token, uint32_t timeout)
{
    uint8_t crcBuf[2];

    /* Wait for data token */
    if (!waitForDataToken(token, timeout)) {
        return false;
    }

    /* Read the block */
    memset(buf, 0xff, size);
    if (!spi::transfer(address, buf, size, spi::NO_CHIP_SELECT)) {
        return false;
    }

    /* Read the CRC */
    memset(crcBuf, 0xff, sizeof(crcBuf));
    if (!spi::transfer(address, crcBuf, sizeof(crcBuf), spi::NO_CHIP_SELECT)) {
        return false;
    }

    /* TODO(ntamas): check CRC */

    return true;
}

static bool readBlockIntoBuffer(uint32_t block, uint8_t* buf)
{
    bool success = false;

    if (!spi::select(address)) {
        return false;
    }

    /* Send the command to read a block */
    if (sendCommand(CMD_READ_SINGLE_BLOCK, block) != RESPONSE_OK) {
        goto cleanup;
    }

    /* Read the block itself */
    if (!readResponseData(buf, BLOCK_SIZE)) {
        goto cleanup;
    }

    success = true;

cleanup:
    if (!spi::unselect(address)) {
        return false;
    }

    return success;
}

/* ************************************************************************** */
/* LittleFS handler function implementations                                  */
/* ************************************************************************** */

static int sdcard_spi_read(
    const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off,
    void* buffer, lfs_size_t size)
{
    /* TODO(ntamas) */
    return LFS_ERR_IO;
}

static int sdcard_spi_write(
    const struct lfs_config* cfg, lfs_block_t sector, lfs_off_t off,
    const void* buffer, lfs_size_t size)
{
    /* TODO(ntamas) */
    return LFS_ERR_IO;
}

static int sdcard_spi_erase(const struct lfs_config* cfg, lfs_block_t sector)
{
    /* TODO(ntamas) */
    return LFS_ERR_IO;
}

static int sdcard_spi_sync(const struct lfs_config* cfg)
{
    /* No write-through cache so we can return immediately */
    return LFS_ERR_OK;
}
