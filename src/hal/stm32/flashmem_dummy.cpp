#include "config.h"
#include "hal/flashmem.h"

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
    return nullptr;
}

}
