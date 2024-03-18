#include "hal/storage.h"
#include "lfs_filebd.h"

using namespace littlefs;
using namespace teller::hal::storage;

static const size_t NUM_AREAS = area::NUMBER_OF_AREAS;

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
static struct lfs_filebd_config filebd_configs[NUM_AREAS] = {
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
static lfs_filebd_t filebds[NUM_AREAS];

/**
 * @brief Filesystem configuration objects for the simulated storage devices.
 */
static std::shared_ptr<FilesystemConfig> cfg[NUM_AREAS];

/**
 * @brief Filenames of the simulated storage devices.
 */
static const char* filenames[NUM_AREAS] = {
    "flash.bin",
    "sdcard.bin"
};

static bool initArea(area::area_t area_to_init);
static void destroyArea(area::area_t area_to_destroy);

namespace teller::hal::storage {

bool init()
{
    bool ok = true;

    ok &= initArea(area::FLASH_MEMORY);
    ok &= initArea(area::SD_CARD);

    if (!ok) {
        destroy();
    }

    return ok;
}

void destroy()
{
    destroyArea(area::SD_CARD);
    destroyArea(area::FLASH_MEMORY);
}

FilesystemConfig* getFilesystemConfig(area::area_t area)
{
    switch (area) {
    case area::FLASH_MEMORY:
    case area::SD_CARD:
        return cfg[area].get();

    default:
        return nullptr;
    }
}

}

bool initArea(area::area_t area_to_init)
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
    } catch (std::bad_alloc&) {
        result = false;
    }

    return result;
}

void destroyArea(area::area_t area_to_destroy)
{
    if (cfg[area_to_destroy]) {
        lfs_filebd_destroy(&cfg[area_to_destroy]->raw_cfg());
        cfg[area_to_destroy].reset();
    }
}
