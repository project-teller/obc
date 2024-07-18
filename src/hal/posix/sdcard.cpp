#include "hal/sdcard.h"
#include "hal/posix/sdcard_debug.h"
#include "lfs_filebd.h"
#include <memory>
#include <unistd.h>

using namespace littlefs;
using namespace teller::hal;

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

namespace teller::hal::sdcard {

bool init()
{
    return true;
}

void destroy()
{
}

bool setup(void)
{
    return true;
}

std::unique_ptr<FilesystemConfig> createFilesystemConfiguration(void)
{
    try {
        auto new_config = std::make_unique<FilesystemConfig>(
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

        new_config->raw_cfg().context = &filebd;
        if (lfs_filebd_create(&new_config->raw_cfg(), getFilename(), &filebd_config) == LFS_ERR_OK) {
            return new_config;
        }
        /* LCOV_EXCL_START */
    } catch (std::bad_alloc&) {
        /* pass */
    }
    /* LCOV_EXCL_STOP */

    return nullptr;
}

const char* getFilename()
{
    return "sdcard.bin";
}

uint32_t getTotalSize()
{
    return SD_CARD_SIZE;
}

bool readData(uint8_t* buf, uint32_t address, size_t length)
{
    if (address > getTotalSize() - length) {
        length = getTotalSize() - address;
    }

    if (lseek(filebd.fd, address, SEEK_SET) < 0) {
        return false;
    }

    return read(filebd.fd, buf, length) >= 0;
}

}
