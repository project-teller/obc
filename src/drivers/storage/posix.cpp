#include <fstream>

#include "core/telem/generic.h"
#include "drivers/flashmem.h"
#include "drivers/flashmem/posix_debug.h"
#include "drivers/sdcard.h"
#include "drivers/sdcard/posix_debug.h"
#include "drivers/storage.h"
#include "drivers/storage/posix_debug.h"

using namespace littlefs;
using namespace teller::drivers;
using namespace teller::telem;

extern "C" int lfs_filebd_destroy(const struct lfs_config* cfg);

/**
 * @brief Filesystem configuration objects for the simulated storage devices.
 */
static std::unique_ptr<FilesystemConfig> cfg[NUM_STORAGE_AREAS];

static bool initArea(storage_area_t area_to_init);
static void destroyArea(storage_area_t area_to_destroy);

namespace teller::drivers::storage {

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
        const char* filename;

        if (i == STORAGE_AREA_FLASH_MEMORY) {
            filename = flashmem::getFilename();
        } else if (i == STORAGE_AREA_SD_CARD) {
            filename = sdcard::getFilename();
        } else {
            filename = nullptr;
        }

        if (filename && std::ifstream(filename).good()) {
            std::remove(filename);
        }
    }
}

}

bool initArea(storage_area_t area_to_init)
{
    std::unique_ptr<FilesystemConfig> this_cfg;

    switch (area_to_init) {
    case STORAGE_AREA_FLASH_MEMORY:
        this_cfg = flashmem::createFilesystemConfiguration();
        break;

    case STORAGE_AREA_SD_CARD:
        this_cfg = sdcard::createFilesystemConfiguration();
        break;

    default:
        break;
    }

    if (this_cfg) {
        cfg[area_to_init] = std::move(this_cfg);
        return true;
    } else {
        return false;
    }
}

void destroyArea(storage_area_t area_to_destroy)
{
    if (cfg[area_to_destroy]) {
        lfs_filebd_destroy(&cfg[area_to_destroy]->raw_cfg());
        cfg[area_to_destroy].reset();
    }
}
