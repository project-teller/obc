#include <cstdint>

#include "core/telem/generic.h"
#include "modules/cmd.h"
#include "modules/supervisor.h"
#include "tasks/cmd.h"

using namespace teller::supervisor;
using namespace teller::telem;

/**
 * @def UPDATE_FREQ_HZ
 * @brief Specifies the update frequency of the command parser task.
 */
#define UPDATE_FREQ_HZ 5

namespace teller::tasks {

[[noreturn]] void commandTask(void* args_)
{
    TaskRegistration task("cmd");
    task.expect(UPDATE_FREQ_HZ - 1, 1000);

    cmd_task_args_t* args = static_cast<cmd_task_args_t*>(args_);
    uint8_t responseBuffer[MAX_PAYLOAD_LENGTH];

    while (true) {
        task.nudge();
        teller::cmd::handleCommands(args->uart_index, responseBuffer, 1000 / UPDATE_FREQ_HZ);
    }
}

}
