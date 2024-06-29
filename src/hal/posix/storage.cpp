#include <fstream>

#include "hal/storage.h"
#include "lfs_filebd.h"

using namespace littlefs;
using namespace teller::hal::storage;
using namespace teller::telem;

/**
 * @brief Block size of the simulated flash memory device
 */
#define FLASH_MEMORY_BLOCK_SIZE 512

/**
 * @brief Total size of the simulated flash memory device
 */
#define FLASH_MEMORY_SIZE (4 * 1024 * 1024)

/**
 * @brief Block size of the simulated SD card
 */
#define SD_CARD_BLOCK_SIZE 512

/**
 * @brief Total size of the simulated SD card
 */
#define SD_CARD_SIZE (4 * 1024 * 1024)

/**
 * @brief Block device configurations for the simulated storage devices.
 */
static struct lfs_filebd_config filebd_configs[NUM_STORAGE_AREAS] = {
    {},
    { .read_size = FLASH_MEMORY_BLOCK_SIZE,
        .prog_size = FLASH_MEMORY_BLOCK_SIZE,
        .erase_size = 4 * FLASH_MEMORY_BLOCK_SIZE,
        .erase_count = FLASH_MEMORY_SIZE / (4 * FLASH_MEMORY_BLOCK_SIZE) },
    { .read_size = SD_CARD_BLOCK_SIZE,
        .prog_size = SD_CARD_BLOCK_SIZE,
        .erase_size = 4 * SD_CARD_BLOCK_SIZE,
        .erase_count = SD_CARD_SIZE / (4 * SD_CARD_BLOCK_SIZE) },
};

/**
 * @brief Block devices for the simulated storage devices.
 */
static lfs_filebd_t filebds[NUM_STORAGE_AREAS];

/**
 * @brief Filesystem configuration objects for the simulated storage devices.
 */
static std::shared_ptr<FilesystemConfig> cfg[NUM_STORAGE_AREAS];

/**
 * @brief Filenames of the simulated storage devices.
 */
static const char* filenames[NUM_STORAGE_AREAS] = {
    nullptr,
    "flash.bin",
    "sdcard.bin"
};

static bool initArea(storage_area_t area_to_init);
static void destroyArea(storage_area_t area_to_destroy);

namespace teller::hal::storage {

bool init()
{
    bool ok = true;

    ok &= initArea(STORAGE_AREA_FLASH_MEMORY);
    ok &= initArea(STORAGE_AREA_SD_CARD);

    if (!ok) {
        destroy();
    }

    return ok;
}

void destroy()
{
    destroyArea(STORAGE_AREA_SD_CARD);
    destroyArea(STORAGE_AREA_FLASH_MEMORY);
}

FilesystemConfig* getFilesystemConfig(storage_area_t area)
{
    switch (area) {
    case STORAGE_AREA_FLASH_MEMORY:
    case STORAGE_AREA_SD_CARD:
        return cfg[area].get();

    default:
        return nullptr;
    }
}

void removeAllFiles(void)
{
    for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
        if (filenames[i] && std::ifstream(filenames[i]).good()) {
            std::remove(filenames[i]);
        }
    }
}

}

bool initArea(storage_area_t area_to_init)
{
    bool result;

    try {
        auto new_config = std::make_shared<FilesystemConfig>(
            lfs_filebd_read,
            lfs_filebd_prog,
            lfs_filebd_erase,
            lfs_filebd_sync,
            filebd_configs[area_to_init].read_size,
            filebd_configs[area_to_init].prog_size,
            filebd_configs[area_to_init].erase_size,
            filebd_configs[area_to_init].erase_count,
            500,
            filebd_configs[area_to_init].read_size,
            filebd_configs[area_to_init].read_size);

        new_config->raw_cfg().context = &filebds[area_to_init];
        result = lfs_filebd_create(
                     &new_config->raw_cfg(),
                     filenames[area_to_init],
                     &filebd_configs[area_to_init])
            == LFS_ERR_OK;

        if (result) {
            cfg[area_to_init] = new_config;
        }
        /* LCOV_EXCL_START */
    } catch (std::bad_alloc&) {
        result = false;
    }
    /* LCOV_EXCL_STOP */

    return result;
}

void destroyArea(storage_area_t area_to_destroy)
{
    if (cfg[area_to_destroy]) {
        lfs_filebd_destroy(&cfg[area_to_destroy]->raw_cfg());
        cfg[area_to_destroy].reset();
    }
}
