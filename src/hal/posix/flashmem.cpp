#include "hal/flashmem.h"

using namespace teller::hal;

namespace teller::hal::flashmem {

bool init()
{
    return true;
}

void destroy()
{
}

bool setup(void)
{
    return true;
}

std::unique_ptr<littlefs::FilesystemConfig> createFilesystemConfiguration(void)
{
    /* Not needed; in the POSIX HAL the storage module will take care of this */
    return nullptr;
}

}
