#include "hal/storage.h"
#include "hal/flashmem.h"

#include "stm32_hal.h"
#include <littlefs-cpp.h>

using namespace littlefs;
using namespace teller::hal;
using namespace teller::telem;

/**
 * @brief Filesystem configuration objects for the simulated storage devices.
 */
static std::unique_ptr<FilesystemConfig> cfg[NUM_STORAGE_AREAS];

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
    return cfg[area].get();
}

}

bool initArea(storage_area_t area_to_init)
{
    bool result = false;

    try {
        if (area_to_init == STORAGE_AREA_FLASH_MEMORY) {
            cfg[area_to_init] = flashmem::createFilesystemConfiguration();
            result = true;
        }
    } catch (std::bad_alloc&) {
        /* nothing to do */
    }

    return result;
}

void destroyArea(storage_area_t area_to_destroy)
{
    if (cfg[area_to_destroy]) {
        cfg[area_to_destroy].reset();
    }
}
