#include <cstdint>

#include "core/telem/generic.h"
#include "hal/system.h"
#include "hal/uart.h"
#include "modules/cmd.h"
#include "modules/supervisor.h"
#include "tasks/cmd.h"

using namespace teller::supervisor;
using namespace teller::telem;
using namespace teller::hal::uart;

/**
 * @def UPDATE_FREQ_HZ
 * @brief Specifies the update frequency of the command parser task.
 */
#define UPDATE_FREQ_HZ 5

namespace teller::tasks {

[[noreturn]] void commandTask(void* args_)
{
    uint8_t responseBuffer[MAX_PAYLOAD_LENGTH];
    cmd_task_args_t* args = static_cast<cmd_task_args_t*>(args_);
    if (!isConnected(args->uart_index)) {
        teller::hal::system::sleepForever();
    }

    while (true) {
        teller::cmd::handleCommands(args->uart_index, responseBuffer);
    }
}

}
