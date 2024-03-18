#include "hal/storage.h"

#include "stm32_hal.h"

using namespace teller::hal::storage;

namespace teller::hal::storage {

bool init()
{
    /* TODO(ntamas): add SD card and flash memory */
    return true;
}

void destroy()
{
}

littlefs::FilesystemConfig* getFilesystemConfig(area::area_t area)
{
    return nullptr;
}

}
