#include <cassert>

#include "tasks/flashmem.h"

#include "core/telem/generic.h"
#include "drivers/flashmem.h"
#include "modules/edr.hpp"

[[noreturn]] void teller::tasks::flashMemoryTask(void* arg)
{
    teller::drivers::flashmem::setup();
    teller::edr::manage(teller::telem::STORAGE_AREA_FLASH_MEMORY);
}
