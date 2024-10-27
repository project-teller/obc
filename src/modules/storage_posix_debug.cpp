#include <fstream>

#include "core/telem/generic.h"
#include "drivers/flashmem.h"
#include "drivers/flashmem/posix_debug.h"
#include "drivers/sdcard.h"
#include "drivers/sdcard/posix_debug.h"
#include "modules/storage_posix_debug.h"

using namespace teller::drivers;
using namespace teller::telem;

namespace teller::storage {

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
