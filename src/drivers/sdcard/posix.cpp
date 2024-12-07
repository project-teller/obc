#include "drivers/sdcard.h"
#include "drivers/sdcard/posix_debug.h"
#include "lfs_filebd.h"
#include <memory>
#include <unistd.h>

using namespace littlefs;

/**
 * @brief Block size of the simulated SD card
 */
#define SD_CARD_BLOCK_SIZE 512

/**
 * @brief Total size of the simulated SD card
 */
#define SD_CARD_SIZE (4 * 1024 * 1024)

static struct lfs_filebd_config filebd_config = { .read_size = SD_CARD_BLOCK_SIZE,
    .prog_size = SD_CARD_BLOCK_SIZE,
    .erase_size = 4 * SD_CARD_BLOCK_SIZE,
    .erase_count = SD_CARD_SIZE / (4 * SD_CARD_BLOCK_SIZE) };

/**
 * @brief Block device for the simulated SD card.
 */
static lfs_filebd_t filebd;

/**
 * @brief Filesystem configuration for the SD card.
 */
static std::unique_ptr<FilesystemConfig> fsCfg;

/**
 * @brief The current operation being performed by the SD card.
 */
static teller::drivers::StorageOperation currentOperation;

static teller::drivers::StorageStatistics stats;

namespace teller::drivers::sdcard {

bool init()
{
    currentOperation = OP_IDLE;
    return true;
}

void destroy()
{
    currentOperation = OP_IDLE;
    fsCfg.reset();
}

FilesystemConfig* setup(void)
{
    try {
        fsCfg = std::make_unique<FilesystemConfig>(
            lfs_filebd_read,
            lfs_filebd_prog,
            lfs_filebd_erase,
            lfs_filebd_sync,
            filebd_config.read_size,
            filebd_config.prog_size,
            filebd_config.erase_size,
            filebd_config.erase_count,
            500,
            filebd_config.read_size,
            filebd_config.read_size);

        fsCfg->raw_cfg().context = &filebd;
        if (lfs_filebd_create(&fsCfg->raw_cfg(), getFilename(), &filebd_config) == LFS_ERR_OK) {
            return fsCfg.get();
        }
        /* LCOV_EXCL_START */
    } catch (std::bad_alloc&) {
        /* pass */
    }
    /* LCOV_EXCL_STOP */

    return nullptr;
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

const char* getFilename()
{
    return "sdcard.bin";
}

uint64_t getTotalSize()
{
    return SD_CARD_SIZE;
}

bool readData(uint8_t* buf, uint64_t address, size_t length)
{
    bool result = false;

    /* Do not reuse filefd.fd here -- we do not want to mess around with the
     * internals of LittleFS behind its back.
     *
     * Also, the image file on the disk is sparse and may be smaller than the
     * logical size of the volume. We need to account for that; if the address
     * is beyond the end of the physical image file, we fill the rest of the
     * buffer with zeros.
     */
    memset(buf, 0, length);

    FILE* fp = fopen(getFilename(), "rb");
    if (!fp) {
        goto cleanup;
    }

    if (fseek(fp, address, SEEK_SET)) {
        goto cleanup;
    }

    if (fread(buf, sizeof(uint8_t), length, fp) < length) {
        result = feof(fp) ? true : false;
    } else {
        result = true;
    }

cleanup:
    if (fp != nullptr) {
        fclose(fp);
    }
    return result;
}

}
