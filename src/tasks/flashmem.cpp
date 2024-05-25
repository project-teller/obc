#include <cassert>

#include "tasks/flashmem.h"

#include "core/telem/generic.h"
#include "modules/edr.hpp"

[[noreturn]] void teller::tasks::flashMemoryTask(void* arg)
{
    teller::edr::manage("flashfs", teller::telem::STORAGE_AREA_FLASH_MEMORY);
}
