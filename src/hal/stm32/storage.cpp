#include "hal/storage.h"

#include "stm32_hal.h"
#include <littlefs-cpp.h>

using namespace littlefs;
using namespace teller::hal::storage;
using namespace teller::telem;

/**
 * @brief Filesystem configuration objects for the simulated storage devices.
 */
static std::shared_ptr<FilesystemConfig> cfg[NUM_STORAGE_AREAS];

static bool initArea(storage_area_t area_to_init);
static void destroyArea(storage_area_t area_to_destroy);

namespace teller::hal::storage {

bool init()
{
    bool ok = true;

    ok &= initArea(STORAGE_AREA_FLASH_MEMORY);
    // ok &= initArea(STORAGE_AREA_SD_CARD);

    if (!ok) {
        destroy();
    }

    return ok;
}

void destroy()
{
    // destroyArea(STORAGE_AREA_SD_CARD);
    destroyArea(STORAGE_AREA_FLASH_MEMORY);
}

littlefs::FilesystemConfig* getFilesystemConfig(storage_area_t area)
{
    return nullptr;
}

}

bool initArea(storage_area_t area_to_init)
{
    bool result = false;

    if (area_to_init != STORAGE_AREA_FLASH_MEMORY) {
        return false;
    }

    result = true;

    /*
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
        cfg[area_to_init] = new_config;
    } catch (std::bad_alloc&) {
        result = false;
    }
    */

    return result;
}

void destroyArea(storage_area_t area_to_destroy)
{
    if (cfg[area_to_destroy]) {
        cfg[area_to_destroy].reset();
    }
}
