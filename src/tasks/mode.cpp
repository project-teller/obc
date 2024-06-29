#include <cstdint>

#include "modules/mode.h"
#include "tasks/mode.h"

namespace teller::tasks {

[[noreturn]] void modeManagerTask(void* args_)
{
    while (true) {
        teller::mode::updateMode();
    }
}

}
