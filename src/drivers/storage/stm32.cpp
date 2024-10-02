#include "drivers/flashmem.h"
#include "drivers/sdcard.h"
#include "drivers/storage.h"

#include "stm32_hal.h"
#include <littlefs-cpp.h>

using namespace littlefs;
using namespace teller::drivers;
using namespace teller::telem;

/**
 * @brief Filesystem configuration objects for the simulated storage devices.
 */
static std::unique_ptr<FilesystemConfig> cfg[NUM_STORAGE_AREAS];

static bool initArea(storage_area_t area_to_init);
static void destroyArea(storage_area_t area_to_destroy);

namespace teller::drivers::storage {

bool init()
{
    initArea(STORAGE_AREA_FLASH_MEMORY);
    initArea(STORAGE_AREA_SD_CARD);

    return true;
}

void destroy()
{
    destroyArea(STORAGE_AREA_SD_CARD);
    destroyArea(STORAGE_AREA_FLASH_MEMORY);
}

littlefs::FilesystemConfig* getFilesystemConfig(storage_area_t area)
{
    return cfg[area].get();
}

}

bool initArea(storage_area_t area_to_init)
{
    try {
        if (area_to_init == STORAGE_AREA_FLASH_MEMORY) {
            cfg[area_to_init] = flashmem::createFilesystemConfiguration();
        } else if (area_to_init == STORAGE_AREA_SD_CARD) {
            cfg[area_to_init] = sdcard::createFilesystemConfiguration();
        } else {
            return false;
        }
    } catch (std::bad_alloc&) {
        /* nothing to do */
    }

    return cfg[area_to_init] != nullptr;
}

void destroyArea(storage_area_t area_to_destroy)
{
    if (cfg[area_to_destroy]) {
        cfg[area_to_destroy].reset();
    }
}
