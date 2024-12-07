#include "config.h"
#include "drivers/sdcard.h"

static teller::drivers::StorageStatistics stats;

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

StorageOperation getCurrentOperation()
{
    return OP_IDLE;
}

/**
 * @brief Returns the statistics of the flash memory.
 */
StorageStatistics getStatistics(void)
{
    return stats;
}

uint64_t getTotalSize()
{
    return 0;
}

bool readData(uint8_t* buf, uint64_t address, size_t length)
{
    memset(buf, 0, length);
    return true;
}

}
