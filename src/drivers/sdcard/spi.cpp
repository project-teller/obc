#include <limits>

#include "config.h"
#include "core/utils/block_cache.h"
#include "core/utils/crc.h"
#include "drivers/sdcard.h"
#include "hal/spi.h"
#include "hal/system.h"
#include "modules/debug.h"
#include "modules/log.h"

#include "stm32_hal.h"

using namespace littlefs;
using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;
using teller::utils::BlockCache;

#if defined(TELLER_BOARD_NUCLEO144)
static const spi::address_t address = spi::NO_ADDRESS;
#elif defined(TELLER_BOARD_STM32F4)
static const spi::address_t address = { .bus = 1, .device = 0 };
#else
static const spi::address_t address = spi::NO_ADDRESS;
#endif

/**
 * Default timeout to use when sending SD card commands or reading data.
 * This is according to section 4.6.2.1 of the physical spec.
 */
static const uint32_t DEFAULT_SD_CARD_TIMEOUT = 100;

/**
 * Default timeout to use for write operations on the SD card.
 * This is according to section 4.6.2.2 of the physical spec.
 */
static const uint32_t DEFAULT_SD_CARD_WRITE_TIMEOUT = 250;

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
#define DATA_TOKEN_RESPONSE_OK 0x05
#define DATA_TOKEN_RESPONSE_CRC_ERROR 0x0B
#define DATA_TOKEN_RESPONSE_WRITE_ERROR 0x0D

static void convertAddressToBlockAndOffset(
    uint32_t address, uint32_t& block, uint32_t& offset);
static const uint8_t* ensureBlockIsCached(uint32_t block);
static bool eraseBlock(uint32_t block);
static void prepareCommand(uint8_t* buf, uint8_t cmd, uint32_t arg);
static bool programBlockFromBuffer(uint32_t block, uint8_t* buf);
static bool readBlockIntoBuffer(uint32_t block, uint8_t* buf);
static bool readDataBlock(
    uint8_t* buf, size_t size, uint8_t token = DATA_TOKEN,
    uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static uint8_t sendCommand(
    uint8_t cmd, uint32_t arg = 0, uint8_t* buf = nullptr, uint8_t size = 0,
    uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static uint8_t sendCommandAssumingNotBusy(
    uint8_t cmd, uint32_t arg = 0, uint8_t* buf = nullptr, uint8_t size = 0,
    uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static bool sendDataBlock(
    uint8_t* buf, size_t size, uint8_t token = DATA_TOKEN,
    uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static uint32_t tryInitialization(void);
static uint8_t waitForToken(uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static bool waitForDataToken(uint32_t timeout = DEFAULT_SD_CARD_TIMEOUT);
static uint8_t waitForDataTokenResponse(uint32_t timeout = DEFAULT_SD_CARD_WRITE_TIMEOUT);
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

/** Block cache for the driver */
static BlockCache blockCache(BLOCK_SIZE);

/** Write buffer for a single block */
static uint8_t writeBuf[BLOCK_SIZE];

/** Stores the number of blocks on the SD card. Zero if no card was found. */
static uint32_t blockCount;

/**
 * @brief Filesystem configuration for the flash memory.
 */
static std::unique_ptr<FilesystemConfig> fsCfg;

namespace teller::drivers::sdcard {

bool init()
{
    bool success;

    logger = getLogger(MODULE_ID_EDR);
    success = (logger != nullptr);

    if (!success) {
        destroy();
    }

    return success;
}

void destroy()
{
    fsCfg.reset();
    blockCount = 0;
}

FilesystemConfig* setup(void)
{
    const char* name = getStorageAreaName(STORAGE_AREA_SD_CARD);

    blockCount = tryInitialization();

    if (blockCount <= 0) {
        if (logger) {
            logger->error("%s: not found", name);
        }
        return nullptr;
    }

    if (logger) {
        logger->info("%s: found (%d MB)", name, (blockCount >> 11));
    }

    fsCfg = std::make_unique<FilesystemConfig>(
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
    return fsCfg.get();
}

uint32_t getTotalSize()
{
    return blockCount * BLOCK_SIZE;
}

bool readData(uint8_t* buf, uint32_t address, size_t length)
{
    /* The address is byte-aligned, and the region to read may span multiple
     * blocks, but we can only read entire blocks */
    uint32_t block, offset;
    uint16_t toReadNow;
    const uint8_t* cachePtr;

    convertAddressToBlockAndOffset(address, block, offset);

    while (length > 0) {
        toReadNow = BLOCK_SIZE - offset;
        if (length < toReadNow) {
            toReadNow = length;
        }

        cachePtr = ensureBlockIsCached(block);
        if (!cachePtr) {
            return false;
        }

        memcpy(buf, cachePtr + offset, toReadNow);

        length -= toReadNow;
        buf += toReadNow;

        block++;
        offset = 0;
    }

    return true;
}

}

/**
 * @brief Converts an address on the SD card to the index of the corresponding block and offset.
 *
 * @param address  the address to convert
 * @return the index of the block containing the address
 */
static void convertAddressToBlockAndOffset(uint32_t address, uint32_t& block, uint32_t& offset)
{
    block = address / BLOCK_SIZE;
    offset = address % BLOCK_SIZE;
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
 * @brief Reads a block of bytes from the device, expecting a data token in
 *        front of the block and a 16-bit XMODEM CRC at the end.
 *
 * Also validates the CRC at the end of the block.
 *
 * The chip select line is assumed to be LOW when this function is called.
 *
 * @param buf      the buffer to read the block into
 * @param size     expected size of the block to read
 * @param token    expected data token in front of the data block
 * @param timeout  timeout for the operation in milliseconds
 * @return whether the read was successful
 */
static bool readDataBlock(uint8_t* buf, size_t size, uint8_t token, uint32_t timeout)
{
    uint8_t crcBuf[2];

    /* Wait for data token */
    if (!waitForDataToken(timeout)) {
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

    /* Compare the observed CRC with the expected one */
    return ((crcBuf[0] << 8) | crcBuf[1]) == crc_xmodem(0, buf, size);
}

/**
 * @brief Sends a standard 6-byte SD card command and reads the response.
 *
 * This function waits until the card reports not being busy before sending the
 * command.
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
    if (waitWhileBusy(timeout)) {
        return sendCommandAssumingNotBusy(cmd, arg, buf, size, timeout);
    } else {
        return 0xFF;
    }
}

/**
 * @brief Sends a standard 6-byte SD card command and reads the response.
 *
 * This function assumes that the card is not busy. Used at initialization time
 * where apparently it is needed for the first command that sends the card to
 * idle state.
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
static uint8_t sendCommandAssumingNotBusy(
    uint8_t cmd, uint32_t arg, uint8_t* buf, uint8_t size, uint32_t timeout)
{
    uint8_t txBuf[10];
    uint8_t rxBuf[10];
    uint8_t i;
    uint8_t response = 0xff;

    if (size > sizeof(rxBuf)) {
        size = sizeof(rxBuf);
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
         * at most 8 bytes. We send the bytes one by one, reading one byte in
         * the response until we get a byte where the MSB is zero -- this will
         * be the response code. Note that 8 bytes may seem overkill, but it
         * was recommended here:
         *
         * https://electronics.stackexchange.com/questions/602105/
         */
        for (i = 0; i < 8; i++) {
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
    if (buf != nullptr && size > 0 && response != 0xff) {
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

/**
 * @brief Sends a block of bytes to the device, prepending a data token in
 *        front of the block and appending a 16-bit XMODEM CRC.
 *
 * The chip select line is assumed to be LOW when this function is called.
 *
 * @param buf      the buffer to write
 * @param size     size of the block to write
 * @param token    data token in front of the data block
 * @param timeout  timeout for the operation in milliseconds
 * @return whether the write was successful
 */
static bool sendDataBlock(uint8_t* buf, size_t size, uint8_t token, uint32_t timeout)
{
    uint8_t crcBuf[2];
    uint16_t crc;

    /* Send the data token */
    if (!spi::transfer(address, &token, 1, spi::NO_CHIP_SELECT)) {
        return false;
    }

    /* Send the block itself */
    if (!spi::transfer(address, buf, BLOCK_SIZE, spi::NO_CHIP_SELECT)) {
        return false;
    }

    /* Send the CRC */
    crc = crc_xmodem(0, buf, BLOCK_SIZE);
    crcBuf[0] = crc >> 8;
    crcBuf[1] = crc & 0xff;
    if (!spi::transfer(address, crcBuf, sizeof(crcBuf), spi::NO_CHIP_SELECT)) {
        return false;
    }

    return true;
}

static uint32_t tryInitialization(void)
{
    uint8_t buf[16]; /* 16 bytes are needed for the CSD register */
    const char* name = getStorageAreaName(STORAGE_AREA_SD_CARD);
    spi::DeviceSelector selector(address);

    blockCount = 0;

    /* SPI initialization needs to start at a low clock speed */
    if (!spi::setClockSpeed(address.bus, 200000)) {
        return 0;
    }

    if (!selector.ensureUnselected()) {
        return 0;
    }

    /* Step 1: set MOSI and CS to high and apply 74 or more clock pulses to
     * SCLK */
    memset(buf, 0xff, sizeof(buf));
    if (!spi::transfer(address, buf, 10, spi::NO_CHIP_SELECT)) {
        return 0;
    }

    if (!selector.ensureSelected()) {
        return 0;
    }

    /* Step 2: send reset command and read response */
    if (sendCommandAssumingNotBusy(CMD_GO_IDLE_STATE) != RESPONSE_IDLE) {
        logger->error("%s: reset failed", name);
        return 0;
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
        return 0;
    }
    if ((buf[2] & 0x01) != 1 || buf[3] != 0xaa) {
        logger->error("%s: not an SD card", name);
        return 0;
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
            return 0;
        }
    }

    /* Step 5: read OCR register. We are expecting an R3 response, which is
     * 4 bytes long. During initialization, bit 31 of the OCR register
     * (Card Power Up Status) must be 1. At that point we can check bit 30,
     * which should also be 1 for an SDHC/SDXC card. */
    if (sendCommand(CMD_READ_OCR, 0x00, buf, 4) != RESPONSE_OK) {
        return 0;
    }
    if ((buf[0] & 0xc0) != 0xc0) {
        logger->error("%s: not an SDHC/SDXC card", name);
        return 0;
    }

    /* Step 6: read CSD register to determine the size of the card */
    if (sendCommand(CMD_SEND_CSD, 0x00) != RESPONSE_OK) {
        return 0;
    }
    if (!readDataBlock(buf, 16)) {
        return 0;
    }

    /* Parse card size from CSD register */
    if ((buf[0] & 0xC0) != 0x40) {
        /* Not a CSD register value; it must start with 0b01xxxxxx */
        return 0;
    }

    /* Success! */
    blockCount = buf[7] & 0x3F; // two bits are reserved
    blockCount = (blockCount << 8) | buf[8];
    blockCount = (blockCount << 8) | buf[9];
    blockCount = (blockCount + 1) << 10; /* assert BLOCK_SIZE == (1 << 9) */

    /* We can try raising the SPI clock speed now to, say, 800 kHz */
    if (!spi::setClockSpeed(address.bus, 800000)) {
        return 0;
    }

    return blockCount;
}

/**
 * @brief Reads bytes from the device until a token is received.
 *
 * @param timeout  timeout for the operation in milliseconds
 * @return the token that was received
 */
static uint8_t waitForToken(uint32_t timeout)
{
    uint8_t data;
    uint32_t now = system::getTimeSinceBootMsec();
    uint32_t deadline = (timeout < std::numeric_limits<uint32_t>::max())
        ? now + timeout
        : std::numeric_limits<uint32_t>::max();

    do {
        data = 0xff;
        if (!spi::transfer(address, &data, sizeof(data), spi::NO_CHIP_SELECT)) {
            return false;
        }

        if (data != 0xff) {
            break;
        }

        now = system::getTimeSinceBootMsec();
        if (now < deadline) {
            system::delayMsec(1);
        }
    } while (now < deadline);

    return data;
}

/**
 * @brief Reads bytes from the device until a data token is received.
 *
 * @param timeout  timeout for the operation in milliseconds
 * @return whether a data token was received successfully
 */
static bool waitForDataToken(uint32_t timeout)
{
    return waitForToken(timeout) == DATA_TOKEN;
}

/**
 * @brief Reads bytes from the device until a data token response is received.
 *
 * @param timeout  timeout for the operation in milliseconds
 * @return whether a data token was received successfully
 */
static uint8_t waitForDataTokenResponse(uint32_t timeout)
{
    return waitForToken(timeout) & 0b00011111;
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

        if (busy == 0xff) {
            break;
        }

        now = system::getTimeSinceBootMsec();
        if (now < deadline) {
            system::delayMsec(1);
        }
    } while (now < deadline);

    return busy == 0xff;
}

/**
 * @brief Reads the contents of a block from the SD card to the given buffer.
 *
 * The buffer must be large enough to hold the entire block.
 *
 * @param block  the index of the block
 * @param buf    the buffer to fill
 * @return whether the operation was successful
 */
static bool readBlockIntoBuffer(uint32_t block, uint8_t* buf)
{
    spi::DeviceSelector selector(address);
    if (!selector.ensureSelected()) {
        return false;
    }

    /* Send the command to read a block */
    if (sendCommand(CMD_READ_SINGLE_BLOCK, block) != RESPONSE_OK) {
        return false;
    }

    /* TODO(ntamas): in theory, the SD card could send an error response
     * instead of a data token. The error response looks like this:
     *
     * Bit 7: 0
     * Bit 6: 0
     * Bit 5: 0
     * Bit 4: Card is Locked.
     * Bit 3: Out of Range.
     * Bit 2: Card ECC Failed.
     * Bit 1: CC Error.
     * Bit 0: Error.
     *
     * This is not handled yet.
     */

    /* Read the block itself */
    return readDataBlock(buf, BLOCK_SIZE);
}

/**
 * @brief Ensures that the block with the given index is in the block cache.
 */
static const uint8_t* ensureBlockIsCached(uint32_t block)
{
    const uint8_t* ptr = blockCache.getBlock(block);
    uint8_t* buf;

    if (ptr) {
        // Block is already in the cache
        return ptr;
    } else {
        // Block is not in the cache yet so load it
        buf = blockCache.getScratchArea();
        if (readBlockIntoBuffer(block, buf)) {
            return blockCache.commit(block);
        }
    }

    return nullptr;
}

/**
 * @brief Programs the contents of the given buffer into the block with the given index.
 *
 * The buffer is assumed to hold at least a full block. Only one block will be
 * written.
 *
 * @param block  the index of the block
 * @param buf    the buffer to write
 * @return whether the operation was successful
 */
static bool programBlockFromBuffer(uint32_t block, uint8_t* buf)
{
    uint8_t response;
    uint8_t tries = 5;
    bool success = false;

    spi::DeviceSelector selector(address);
    if (!selector.ensureSelected()) {
        return false;
    }

    /* Evict the block from the cache */
    blockCache.evict(block);

    while (tries) {
        /* Send the command to write a block */
        bool sent = sendCommand(CMD_WRITE_SINGLE_BLOCK, block) == RESPONSE_OK && sendDataBlock(buf, BLOCK_SIZE);
        if (sent) {
            /* Wait for a data token response */
            response = waitForDataTokenResponse();
            if (response == DATA_TOKEN_RESPONSE_OK) {
                success = waitWhileBusy(DEFAULT_SD_CARD_WRITE_TIMEOUT);
                break;
            } else if (response == DATA_TOKEN_RESPONSE_CRC_ERROR) {
                /* This is OK */
            } else {
                /* Write error or invalid response */
                break;
            }
        }

        tries--;
    }

    return success;
}

/**
 * @brief Erases the block with the given index.
 *
 * @param block  the index of the block to erase
 * @return whether the operation was successful
 */
static bool eraseBlock(uint32_t block)
{
    spi::DeviceSelector selector(address);
    if (!selector.ensureSelected()) {
        return false;
    }

    /* Evict the block from the cache */
    blockCache.evict(block);

    /* Send the commands to set the range we want to erase */
    if (sendCommand(CMD_ERASE_WR_BLK_START_ADDR, block) != RESPONSE_OK) {
        return false;
    }
    if (sendCommand(CMD_ERASE_WR_BLK_END_ADDR, block) != RESPONSE_OK) {
        return false;
    }

    /* Send the command to erase a block */
    if (sendCommand(CMD_ERASE) != RESPONSE_OK) {
        return false;
    }

    /* Wait until idle */
    waitWhileBusy(DEFAULT_SD_CARD_WRITE_TIMEOUT);

    return true;
}

/* ************************************************************************** */
/* LittleFS handler function implementations                                  */
/* ************************************************************************** */

static int sdcard_spi_read(
    const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off,
    void* buffer, lfs_size_t size)
{
    assert(off == 0);
    return teller::drivers::sdcard::readData(reinterpret_cast<uint8_t*>(buffer), block * BLOCK_SIZE, size)
        ? LFS_ERR_OK
        : LFS_ERR_IO;
}

static int sdcard_spi_write(
    const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off,
    const void* buffer, lfs_size_t size)
{
    assert(off == 0);
    assert(size == BLOCK_SIZE);

    /* Need to copy the buffer to a temporary write buffer because the SPI
     * transaction will modify it */
    memcpy(writeBuf, buffer, BLOCK_SIZE);
    return programBlockFromBuffer(block, writeBuf) ? LFS_ERR_OK : LFS_ERR_IO;
}

static int sdcard_spi_erase(const struct lfs_config* cfg, lfs_block_t block)
{
    return eraseBlock(block) ? LFS_ERR_OK : LFS_ERR_IO;
}

static int sdcard_spi_sync(const struct lfs_config* cfg)
{
    /* No write-through cache so we can return immediately */
    return LFS_ERR_OK;
}
