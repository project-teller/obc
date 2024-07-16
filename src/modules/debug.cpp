#include "modules/debug.h"
#include "config.h"
#include "core/telem/generic.h"
#include "modules/log.h"

#include <cstring>

using namespace teller::log;
using namespace teller::telem;

#ifdef TELLER_BOARD_POSIX
#define NOINIT_SECTION
#else
#define NOINIT_SECTION __attribute__((section(".noinit")))
#endif

/**
 * Stores debugging information that should survive a soft reboot. This is
 * achieved by putting the variable in the .noinit section.
 */
static teller::debug::debug_info_t debug_info NOINIT_SECTION;

namespace teller::debug {

void init()
{
    if (debug_info.first_boot_marker != 0xC99C9CC9) {
        /* This is the first boot */
        debug_info.first_boot_marker = 0xC99C9CC9;
        debug_info.errors = 0;
        memset(&debug_info.task, 0, sizeof(debug_info.task));
    }
}

void destroy()
{
    /* Nothing to do */
}

debug_info_t* getDebugInfo(void)
{
    return &debug_info;
}

uint32_t getAndClearErrorFlags(void)
{
    uint32_t result = debug_info.errors;
    debug_info.errors = 0;
    return result;
}

void reportErrorsDuringPreviousBoot(void)
{
    uint32_t errors = getAndClearErrorFlags();
    Logger* logger = getLogger(MODULE_ID_GENERIC);

    if (errors & ERROR_STACK_OVERFLOW) {
        logger->error("%s: stack overflow", debug_info.task);
    } else if (errors & ERROR_MALLOC_FAILED) {
        logger->error("%s: malloc failed", debug_info.task);
    }
}

}
