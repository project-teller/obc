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
    cmd_task_args_t* args = static_cast<cmd_task_args_t*>(args_);
    if (!isConnected(args->uart_index)) {
        teller::hal::system::sleepForever();
    }

    uint8_t responseBuffer[MAX_PAYLOAD_LENGTH];
    TaskRegistration task(args->task_name);

    task.expect(UPDATE_FREQ_HZ - 1, 1000);
    while (true) {
        task.nudge();
        teller::cmd::handleCommands(args->uart_index, responseBuffer, 1000 / UPDATE_FREQ_HZ);
    }
}

}
