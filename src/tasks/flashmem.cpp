#include <cassert>

#include "tasks/flashmem.h"

#include "hal/led.h"
#include "hal/storage.h"
#include "hal/system.h"

#include "modules/edr.h"
#include "modules/log.h"
#include "modules/storage.h"

using namespace teller::edr;
using namespace teller::hal;
using namespace teller::telem;

[[noreturn]] void teller::tasks::flashMemoryTask(void* arg)
{
    teller::log::Logger* log = teller::log::getLogger(MODULE_ID_GENERIC);

    for (;;) {
        littlefs::Filesystem* fs;

        fs = storage::waitUntilMounted(STORAGE_AREA_FLASH_MEMORY);
        log->info("flashfs mounted");

        try {
            ExperimentDataRecorder(fs).run();
        } catch (...) {
            /* pass */
        }

        storage::unmountStorage(STORAGE_AREA_FLASH_MEMORY);
        log->warning("flashfs unmounted, waiting for remount");
    }
}
