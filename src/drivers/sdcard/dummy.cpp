#include "config.h"
#include "drivers/sdcard.h"

namespace teller::drivers::sdcard {

bool init()
{
    return true;
}

void destroy()
{
}

littlefs::FilesystemConfig* setup(void)
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
