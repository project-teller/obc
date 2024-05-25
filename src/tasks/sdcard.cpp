#include <cassert>

#include "tasks/sdcard.h"

#include "core/telem/generic.h"
#include "modules/edr.hpp"

[[noreturn]] void teller::tasks::sdCardTask(void* arg)
{
    teller::edr::manage("sdfs", teller::telem::STORAGE_AREA_SD_CARD);
}
