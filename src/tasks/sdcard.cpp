#include <cassert>

#include "tasks/sdcard.h"

#include "core/telem/generic.h"
#include "drivers/sdcard.h"
#include "modules/edr.hpp"

[[noreturn]] void teller::tasks::sdCardTask(void* arg)
{
    teller::drivers::sdcard::setup();
    teller::edr::manage(teller::telem::STORAGE_AREA_SD_CARD);
}
