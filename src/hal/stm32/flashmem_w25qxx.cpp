#include "config.h"
#include "hal/flashmem.h"
#include "hal/spi.h"
#include "hal/system.h"
#include "modules/log.h"

using namespace littlefs;
using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

static const spi::address_t address = {
    .bus = 0,
    .device = 0
};

/* Constants for size conversions */
#define BLOCK_SIZE_IN_KB 64
#define SECTOR_SIZE_IN_KB 4
#define BLOCK_SIZE (BLOCK_SIZE_IN_KB * 1024)
#define SECTOR_SIZE (SECTOR_SIZE_IN_KB * 1024)
#define PAGE_SIZE 256
#define SECTORS_IN_BLOCK (BLOCK_SIZE_IN_KB / SECTOR_SIZE_IN_KB)

/* Commands in instruction set 1 */
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
#define CMD_BLOCK_ERASE_64K 0xD8

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
    { "W25Q512", 0x401A, 1024 },
    NO_MORE_ENTRIES
};

static bool eraseSector(uint32_t sector);
static const flashmem_w25qxx_cfg_t* identify(void);
static bool readData(uint32_t offset, uint8_t* buf, uint16_t length);
static uint32_t readJEDECId(void);
static bool waitWhileBusy(void);
static bool writePage(uint32_t offset, const uint8_t* buf, uint8_t length);

/** The logger used by the driver */
static Logger* logger;

/** Configuration of the detected flash memory variant */
const flashmem_w25qxx_cfg_t* cfg;

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
                enabled = true;
            }
        }
        return enabled;
    }

private:
    bool enabled = false;
};

namespace teller::hal::flashmem {

bool init()
{
    bool success;

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
    cfg = nullptr;
    logger = nullptr;
}

std::unique_ptr<FilesystemConfig> createFilesystemConfiguration(void)
{
    if (!cfg) {
        return nullptr;
    }

    auto new_config = std::make_unique<FilesystemConfig>(
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

    return new_config;
}

bool setup(void)
{
    if (!logger) {
        return false;
    }

    if (cfg) {
        logger->info("Flash: %s (%dKB)", cfg->name, cfg->block_count * BLOCK_SIZE_IN_KB);
    } else {
        logger->error("Flash: not found");
    }

    return true;
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
    uint8_t buf[4] = { CMD_READ_JEDEC_ID, 0x00, 0x00, 0x00 };
    if (!spi::transfer(address, buf, sizeof(buf))) {
        return 0;
    } else {
        return (buf[1] << 16) | (buf[2] << 8) | buf[3];
    }
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
    uint8_t buf[2] = { CMD_READ_STATUS_REGISTER_1, 0x00 };

    while (true) {
        buf[0] = CMD_READ_STATUS_REGISTER_1;
        buf[1] = 0x00;
        if (!spi::transfer(address, buf, sizeof(buf))) {
            return false;
        }

        if ((buf[1] & 0x01) == 0) {
            break;
        }

        system::delayMsec(1);
    }

    return true;
}

/**
 * @brief Erases the 4K sector with the given index.
 *
 * @param sector  the index of the 4K sector to erase
 * @return whether the erase was successful
 */
static bool eraseSector(uint32_t sector)
{
    if (!cfg || sector >= cfg->block_count) {
        return false;
    }

    uint8_t buf[] = {
        CMD_SECTOR_ERASE_4K,
        static_cast<uint8_t>((sector >> 16) & 0xFF),
        static_cast<uint8_t>((sector >> 8) & 0xFF),
        static_cast<uint8_t>(sector & 0xFF)
    };
    bool success = waitWhileBusy();

    if (success) {
        WriteEnabledContext ctx;
        success = ctx.enable() && spi::transfer(address, buf, sizeof(buf));

        if (!waitWhileBusy()) {
            success = false;
        }
    }

    return success;
}

static bool readData(uint32_t offset, uint8_t* buf, uint16_t length)
{
    uint8_t header[] = {
        CMD_FAST_READ,
        static_cast<uint8_t>((offset >> 16) & 0xFF),
        static_cast<uint8_t>((offset >> 8) & 0xFF),
        static_cast<uint8_t>(offset & 0xFF),
        0
    };
    spi::transfer_t xfer[] = {
        { header, nullptr, sizeof(header) },
        { buf, nullptr, length },
        spi::NO_MORE_TRANSFERS
    };
    return waitWhileBusy() && spi::transfer(address, xfer, 0);
}

static bool writePage(uint32_t offset, const uint8_t* buf, uint8_t length)
{
    uint8_t header[] = {
        CMD_PAGE_PROGRAM,
        static_cast<uint8_t>((offset >> 16) & 0xFF),
        static_cast<uint8_t>((offset >> 8) & 0xFF),
        static_cast<uint8_t>(offset & 0xFF)
    };
    spi::transfer_t xfer[] = {
        { header, nullptr, sizeof(header) },
        { const_cast<uint8_t*>(buf), dummy_rx_buf, length },
        spi::NO_MORE_TRANSFERS
    };
    bool success = waitWhileBusy();

    {
        WriteEnabledContext ctx;
        success = ctx.enable() && spi::transfer(address, xfer, 0);

        if (!waitWhileBusy()) {
            success = false;
        }
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
    uint32_t address = sector * SECTOR_SIZE + off;
    uint16_t toRead;
    const uint16_t maxToRead = std::numeric_limits<uint16_t>::max();

    while (size > 0) {
        toRead = size > maxToRead ? maxToRead : size;
        if (!readData(address, ptr, toRead)) {
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
    uint32_t address = sector * SECTOR_SIZE + off;
    uint16_t toWrite;

    while (size > 0) {
        toWrite = size > PAGE_SIZE ? PAGE_SIZE : size;
        if (!writePage(address, ptr, toWrite)) {
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
    return eraseSector(sector) ? LFS_ERR_OK : LFS_ERR_IO;
}

static int flashmem_sync(const struct lfs_config* cfg)
{
    return waitWhileBusy() ? LFS_ERR_OK : LFS_ERR_IO;
}
