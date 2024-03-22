#include <cstdint>

#include "core/telem/generic.h"
#include "modules/cmd.h"
#include "tasks/cmd.h"

using namespace teller::telem;

namespace teller::tasks {

[[noreturn]] void commandTask(void* args)
{
    uint8_t responseBuffer[MAX_PAYLOAD_LENGTH];

    while (true) {
        teller::cmd::handleCommands(responseBuffer);
    }
}

}
