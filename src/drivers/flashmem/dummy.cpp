#include "config.h"
#include "drivers/flashmem.h"

namespace teller::drivers::flashmem {

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

uint32_t getTotalSize()
{
    return 0;
}

bool readData(uint8_t* buf, uint32_t address, size_t length)
{
    memset(buf, 0, length);
    return true;
}

}
