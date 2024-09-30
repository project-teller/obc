#include "drivers/storage.h"

namespace teller::drivers::storage {

bool init()
{
    return true;
}

void destroy()
{
}

littlefs::FilesystemConfig* getFilesystemConfig(storage_area_t area)
{
    return nullptr;
}

}
