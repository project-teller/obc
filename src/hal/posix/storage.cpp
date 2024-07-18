#include <fstream>

#include "hal/flashmem.h"
#include "hal/posix/flashmem_debug.h"
#include "hal/posix/sdcard_debug.h"
#include "hal/sdcard.h"
#include "hal/storage.h"
#include "lfs_filebd.h"

using namespace littlefs;
using namespace teller::hal::storage;
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
        this_cfg = teller::hal::flashmem::createFilesystemConfiguration();
        break;

    case STORAGE_AREA_SD_CARD:
        this_cfg = teller::hal::sdcard::createFilesystemConfiguration();
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
