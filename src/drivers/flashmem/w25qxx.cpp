#include <cassert>
#include <limits>

#include "config.h"
#include "drivers/flashmem.h"
#include "hal/spi.h"
#include "hal/system.h"
#include "modules/log.h"

using namespace littlefs;
using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
static const spi::address_t address = { .bus = 0, .device = 0 };
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board
static const spi::address_t address = { .bus = 0, .device = 0 };
#else
// No flash memory on this board
static const spi::address_t address = spi::NO_ADDRESS;
#endif

/* Constants for size conversions */
#define BLOCK_SIZE_IN_KB 64
#define SECTOR_SIZE_IN_KB 4
#define BLOCK_SIZE (BLOCK_SIZE_IN_KB * 1024)
#define SECTOR_SIZE (SECTOR_SIZE_IN_KB * 1024)
#define PAGE_SIZE 256
#define SECTORS_IN_BLOCK (BLOCK_SIZE_IN_KB / SECTOR_SIZE_IN_KB)

/* Number of retries for read, write and erase operations*/
#define MAX_RETRIES 5

/* Commands in instruction set 1 */
#define CMD_INVALID 0x00
#define CMD_PAGE_PROGRAM 0x02
#define CMD_READ_DATA 0x03
#define CMD_WRITE_DISABLE 0x04
#define CMD_READ_STATUS_REGISTER_1 0x05
#define CMD_WRITE_ENABLE 0x06
#define CMD_FAST_READ 0x0B
#define CMD_READ_STATUS_REGISTER_3 0x15
#define CMD_SECTOR_ERASE_4K 0x20
#define CMD_READ_STATUS_REGISTER_2 0x35
#define CMD_READ_JEDEC_ID 0x9F
#define CMD_ENTER_4_BYTE_MODE 0xB7
#define CMD_BLOCK_ERASE_64K 0xD8
#define CMD_EXIT_4_BYTE_MODE 0xE9

/* List of supported JEDEC IDs and the corresponding block sizes.
 *
 * The page size of each chip is 256 bytes. Pages can be erased in groups of
 * 16 (4KB, one sector), groups of 128 (32KB), groups of 256 (64KB, this is what
 * we will call one block) or the entire chip.
 */
typedef struct {
    const char* name;
    uint16_t jedec_id;
    uint32_t block_count;
} flashmem_w25qxx_cfg_t;

#define NO_MORE_ENTRIES \
    {                   \
        0               \
    }

/**
 * @brief Table storing the JEDEC IDs and names of supported devices along with
 * their basic configuration.
 */
static const flashmem_w25qxx_cfg_t known_devices[] = {
    { "W25Q10", 0x4011, 2 },
    { "W25Q20", 0x4012, 4 },
    { "W25Q40", 0x4013, 8 },
    { "W25Q80", 0x4014, 16 },
    { "W25Q16", 0x4015, 32 },
    { "W25Q32", 0x4016, 64 },
    { "W25Q64", 0x4017, 128 },
    { "W25Q128", 0x4018, 256 },
    { "W25Q256", 0x4019, 512 },
    { "W25Q512", 0x4020, 1024 },
    { "W25Q01", 0x4021, 2048 },
    NO_MORE_ENTRIES
};

static bool eraseSector(uint32_t sector);
static const flashmem_w25qxx_cfg_t* identify(void);
static bool isWriteEnabled(void);
static bool programFromBuffer(uint32_t offset, const uint8_t* buf, uint16_t length);
static bool readIntoBuffer(uint32_t offset, uint8_t* buf, uint16_t length);
static uint32_t readJEDECId(void);
static int16_t readStatusRegister(uint8_t index);
static uint32_t sectorToAddress(uint32_t sector, uint32_t off = 0);
static bool setFourByteAddressingMode(bool enabled = true);
static bool waitWhileBusy(void);

/** The logger used by the driver */
static Logger* logger;

/** Configuration of the detected flash memory variant */
const flashmem_w25qxx_cfg_t* cfg;

/**
 * @brief Filesystem configuration for the flash memory.
 */
static std::unique_ptr<FilesystemConfig> fsCfg;

/**
 * @brief The current operation being performed by the SD card.
 */
static teller::drivers::StorageOperation currentOperation;

static teller::drivers::StorageStatistics stats;

/** Dummy buffer used during SPI transfers to prevent modifying the data
 * structures of LittleFS when writing a page */
static uint8_t dummy_rx_buf[PAGE_SIZE];

/* LittleFS handler functions. Note the confusion in nomenclature: one
 * LittleFS block is actually one 4K _sector_ as blocks are 64KB large */
static int flashmem_read(
    const struct lfs_config* cfg, lfs_block_t sector, lfs_off_t off,
    void* buffer, lfs_size_t size);
static int flashmem_write(
    const struct lfs_config* cfg, lfs_block_t sector, lfs_off_t off,
    const void* buffer, lfs_size_t size);
static int flashmem_erase(const struct lfs_config* cfg, lfs_block_t sector);
static int flashmem_sync(const struct lfs_config* cfg);

/**
 * @brief RAII object that enables writing in its own code block.
 */
class WriteEnabledContext {
public:
    WriteEnabledContext()
        : enabled(false)
    {
    }

    ~WriteEnabledContext()
    {
        uint8_t buf[] = { CMD_WRITE_DISABLE };
        if (enabled) {
            spi::transfer(address, buf, 1);
            /* Return value ignored, there's not much we can do if it fails */
        }
    }

    bool enable()
    {
        uint8_t buf[] = { CMD_WRITE_ENABLE };
        if (!enabled) {
            if (spi::transfer(address, buf, 1)) {
                enabled = isWriteEnabled();
            }
        }
        return enabled;
    }

private:
    bool enabled = false;
};

namespace teller::drivers::flashmem {

bool init()
{
    bool success;

    currentOperation = OP_IDLE;
    logger = getLogger(MODULE_ID_EDR);
    cfg = identify();

    /* cfg == NULL is okay, we still want to report that we did not find the
     * flash memory in setup() */
    success = (logger != nullptr);

    if (!success) {
        destroy();
    }

    return success;
}

void destroy()
{
    fsCfg.reset();
    cfg = nullptr;
    logger = nullptr;
    currentOperation = OP_IDLE;
}

FilesystemConfig* setup(void)
{
    const char* name = getStorageAreaName(STORAGE_AREA_FLASH_MEMORY);

    if (!cfg) {
        if (logger) {
            logger->error("%s: not found", name);
        }
        return nullptr;
    }

    if (logger) {
        logger->info("%s: %s (%d KB)", name, cfg->name, cfg->block_count * BLOCK_SIZE_IN_KB);
    }

    fsCfg = std::make_unique<FilesystemConfig>(
        flashmem_read, /* read */
        flashmem_write, /* prog */
        flashmem_erase, /* erase */
        flashmem_sync, /* sync */
        /* read_size = */ PAGE_SIZE,
        /* prog_size = */ PAGE_SIZE,
        /* erase_size = */ SECTOR_SIZE,
        /* erase_count = */ cfg->block_count * SECTORS_IN_BLOCK,
        /* block_cycles = */ 500,
        /* cache_size = */ PAGE_SIZE,
        /* lookahead_size = */ PAGE_SIZE);
    return fsCfg.get();
}

StorageOperation getCurrentOperation()
{
    return currentOperation;
}

/**
 * @brief Returns the statistics of the flash memory.
 */
StorageStatistics getStatistics(void)
{
    return stats;
}

uint64_t getTotalSize()
{
    return cfg->block_count * BLOCK_SIZE;
}

bool readData(uint8_t* buf, uint64_t address, size_t length)
{
    return readIntoBuffer(address, buf, length);
}
}

/**
 * @brief Identifies the attached device and returns the configuration to use.
 *
 * @return The configuration of the attached device or a null pointer if the
 *         flash memory cannot be identified.
 */
const flashmem_w25qxx_cfg_t* identify()
{
    const flashmem_w25qxx_cfg_t* cfg;
    uint32_t jedec_id = readJEDECId();

    /* Check whether it's a Winbond device */
    if ((jedec_id >> 16) == 0xEF) {
        /* ...then find out its size */
        jedec_id &= 0xFFFF;
        for (cfg = known_devices; cfg->jedec_id; cfg++) {
            if (cfg->jedec_id == jedec_id) {
                if (cfg->block_count >= 512) {
                    if (!setFourByteAddressingMode(true)) {
                        cfg = nullptr;
                    }
                }
                return cfg;
            }
        }
    }

    return nullptr;
}

/**
 * @brief Reads the JEDEC ID of the flash memory.
 */
static uint32_t readJEDECId()
{
    // For some strange reason, stepping through this function with the
    // debugger does not work if we are re-using 'buf' for transmission and
    // reception
    uint8_t buf[4] = { CMD_READ_JEDEC_ID, 0x00, 0x00, 0x00 };
    uint8_t rxBuf[4] = { 0x00, 0x00, 0x00, 0x00 };
    if (!spi::transfer(address, buf, rxBuf, sizeof(buf), 0)) {
        return 0;
    } else {
        return (rxBuf[1] << 16) | (rxBuf[2] << 8) | rxBuf[3];
    }
}

/**
 * @brief Reads the status register with the given index.
 *
 * @return -1 in case of an error, or the value of the status register otherwise
 */
static int16_t readStatusRegister(uint8_t index)
{
    // For some strange reason, stepping through this function with the
    // debugger does not work if we are re-using 'buf' for transmission and
    // reception
    uint8_t buf[2] = { CMD_INVALID, 0 };
    uint8_t rxBuf[2] = { 0, 0 };

    if (index == 1) {
        buf[0] = CMD_READ_STATUS_REGISTER_1;
    } else if (index == 2) {
        buf[0] = CMD_READ_STATUS_REGISTER_2;
    } else if (index == 3) {
        buf[0] = CMD_READ_STATUS_REGISTER_3;
    }

    if (buf[0] != CMD_INVALID && spi::transfer(address, buf, rxBuf, sizeof(buf))) {
        return rxBuf[1];
    } else {
        return -1;
    }
}

/**
 * @brief Returns whether writing to the flash memory is enabled.
 */
static bool isWriteEnabled()
{
    int16_t reg = readStatusRegister(1);
    return reg >= 0 && (reg & 0x02);
}

/**
 * @brief Blocks the current task while the flash memory is busy according to
 * its status register.
 *
 * @return true if the wait was successful, false if there was an error while
 * querying the status register.
 */
static bool waitWhileBusy()
{
    while (true) {
        int16_t reg = readStatusRegister(1);
        if (reg < 0) {
            return false;
        } else if ((reg & 0x01) == 0) {
            return true;
        }
        system::delayMsec(1);
    }
}

/**
 * @brief Converts a sector index to its start address.
 *
 * @param sector  the sector index
 * @param off  the offset within the sector
 * @return the start address of the sector plus the optional offset
 */
static uint32_t sectorToAddress(uint32_t sector, uint32_t off)
{
    return sector * SECTOR_SIZE + off;
}

/**
 * @brief Turns on four-byte addressing mode on the flash memory.
 *
 * Four-byte addressing is required for memories with at least 512 blocks.
 *
 * @param enabled  whether to enable four-byte addresses
 * @return whether the operation was successful
 */
static bool setFourByteAddressingMode(bool enabled)
{
    uint8_t cmd = enabled ? CMD_ENTER_4_BYTE_MODE : CMD_EXIT_4_BYTE_MODE;
    uint8_t response = 0;

    if (!spi::transfer(address, &cmd, &response, 1, 0)) {
        return false;
    }

    return readStatusRegister(3) & 0x01;
}

/**
 * @brief Fills a buffer with the given address.
 *
 * The address will be written in big endian format, in 3 or 4 bytes, depending
 * on the size of the flash memory being handled.
 *
 * @param buf      the buffer to fill
 * @param address  the address to write
 * @return pointer to the first byte after the address that was written
 */
static uint8_t* fillBufferWithAddress(uint8_t* buf, uint32_t address)
{
    if (cfg->block_count >= 512) {
        /* Needs 4-byte addressing */
        *(buf++) = static_cast<uint8_t>((address >> 24) & 0xFF);
    }

    *(buf++) = static_cast<uint8_t>((address >> 16) & 0xFF);
    *(buf++) = static_cast<uint8_t>((address >> 8) & 0xFF);
    *(buf++) = static_cast<uint8_t>(address & 0xFF);

    return buf;
}

/**
 * @brief Erases the 4K sector with the given index.
 *
 * @param sector  the index of the 4K sector to erase
 * @return whether the erase was successful
 */
static bool eraseSector(uint32_t sector)
{
    teller::drivers::OperationContext ctx(&currentOperation, teller::drivers::OP_ERASE);
    bool success = false;
    int retriesSoFar = 0;

    if (!cfg || sector >= cfg->block_count * SECTORS_IN_BLOCK) {
        return false;
    }

    uint32_t offset = sectorToAddress(sector);
    uint8_t buf[6] = { CMD_SECTOR_ERASE_4K };
    uint8_t* end = fillBufferWithAddress(buf + 1, offset);

    while (retriesSoFar < MAX_RETRIES) {
        success = waitWhileBusy();

        if (success) {
            WriteEnabledContext ctx;
            success = ctx.enable() && spi::transfer(address, buf, static_cast<uint16_t>(end - buf));

            if (!waitWhileBusy()) {
                success = false;
            }
        }

        if (success) {
            break;
        }

        retriesSoFar++;
    }

    if (success) {
        stats.blocksErased++;
    }

    if (retriesSoFar > 0) {
        stats.retries++;
    }

    return success;
}

static bool readIntoBuffer(uint32_t offset, uint8_t* buf, uint16_t length)
{
    teller::drivers::OperationContext ctx(&currentOperation, teller::drivers::OP_READ);
    bool success = false;
    int retriesSoFar = 0;

    /* TODO(ntamas): figure out why it does not work with CMD_FAST_READ! */
    /* Maybe the length of the header buffer? It is not divisible by 4 */
    /* when using CMD_FAST_READ */
    // uint8_t header[6] = { CMD_FAST_READ };
    // uint8_t* end = fillBufferWithAddress(header + 1, offset);
    //
    // /* add 8 dummy clock cycles */
    // *(end++) = 0;
    uint8_t header[6] = { CMD_READ_DATA };
    uint8_t* end = fillBufferWithAddress(header + 1, offset);

    spi::transfer_t xfer[] = {
        { header, nullptr, static_cast<uint16_t>(end - header) },
        { nullptr, buf, length },
        spi::NO_MORE_TRANSFERS
    };

    while (retriesSoFar < MAX_RETRIES) {
        success = waitWhileBusy() && spi::transfer(address, xfer, 0);

        if (success) {
            break;
        }

        retriesSoFar++;
    }

    if (success) {
        stats.bytesRead += length;
    }

    if (retriesSoFar > 0) {
        stats.retries++;
    }

    return success;
}

static bool programFromBuffer(uint32_t offset, const uint8_t* buf, uint16_t length)
{
    teller::drivers::OperationContext ctx(&currentOperation, teller::drivers::OP_WRITE);

    /* Programming operation works only on previously erased pages */
    uint8_t header[6] = { CMD_PAGE_PROGRAM };
    uint8_t* end = fillBufferWithAddress(header + 1, offset);
    spi::transfer_t xfer[] = {
        { header, nullptr, static_cast<uint16_t>(end - header) },
        { const_cast<uint8_t*>(buf), dummy_rx_buf, length },
        spi::NO_MORE_TRANSFERS
    };
    bool success = false;
    int retriesSoFar = 0;

    while (retriesSoFar < MAX_RETRIES) {
        success = waitWhileBusy();

        if (success) {
            WriteEnabledContext ctx;
            int code = 0;
            success = ctx.enable();

            if (!success) {
                logger->error("programFromBuffer: failed to enable WEL");
            } else {
                success = spi::transfer(address, xfer, 0);
                if (!success) {
                    code = spi::getLastErrorCode();
                    logger->error("programFromBuffer: HAL error %d", code);
                }

                if (!waitWhileBusy()) {
                    success = false;
                }
            }
        }

        if (success) {
            break;
        }

        retriesSoFar++;
    }

    if (success) {
        stats.bytesWritten += length;
    }

    if (retriesSoFar > 0) {
        stats.retries++;
    }

    return success;
}

/* ************************************************************************** */
/* LittleFS handler function implementations                                  */
/* ************************************************************************** */

static int flashmem_read(
    const struct lfs_config* cfg, lfs_block_t sector, lfs_off_t off,
    void* buffer, lfs_size_t size)
{
    auto ptr = static_cast<uint8_t*>(buffer);
    uint32_t address = sectorToAddress(sector, off);
    uint16_t toRead;
    const uint16_t maxToRead = std::numeric_limits<uint16_t>::max();

    while (size > 0) {
        toRead = size > maxToRead ? maxToRead : size;
        if (!readIntoBuffer(address, ptr, toRead)) {
            return LFS_ERR_IO;
        }
        address += toRead;
        ptr += toRead;
        size -= toRead;
    }

    return LFS_ERR_OK;
}

static int flashmem_write(
    const struct lfs_config* cfg, lfs_block_t sector, lfs_off_t off,
    const void* buffer, lfs_size_t size)
{
    auto ptr = static_cast<const uint8_t*>(buffer);
    uint32_t address = sectorToAddress(sector, off);
    uint16_t toWrite;

    while (size > 0) {
        toWrite = size > PAGE_SIZE ? PAGE_SIZE : size;
        if (!programFromBuffer(address, ptr, toWrite)) {
            return LFS_ERR_IO;
        }
        address += toWrite;
        ptr += toWrite;
        size -= toWrite;
    }

    return LFS_ERR_OK;
}

static int flashmem_erase(const struct lfs_config* cfg, lfs_block_t sector)
{
    if (eraseSector(sector)) {
        return LFS_ERR_OK;
    } else {
        return LFS_ERR_IO;
    }
}

static int flashmem_sync(const struct lfs_config* cfg)
{
    if (waitWhileBusy()) {
        return LFS_ERR_OK;
    } else {
        return LFS_ERR_IO;
    }
}
