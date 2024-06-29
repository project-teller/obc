#include <cstdint>

#include "core/telem/generic.h"
#include "modules/cmd.h"
#include "tasks/cmd.h"

using namespace teller::telem;

namespace teller::tasks {

[[noreturn]] void commandTask(void* args_)
{
    cmd_task_args_t* args = static_cast<cmd_task_args_t*>(args_);
    uint8_t responseBuffer[MAX_PAYLOAD_LENGTH];

    while (true) {
        teller::cmd::handleCommands(args->uart_index, responseBuffer);
    }
}

}
