#include "modules/cmd.h"
#include "modules/supervisor.h"
#include "tasks/cmd.h"

namespace teller::tasks {

[[noreturn]] void commandTask(void* args_)
{
    supervisor::QueueRegistration queue("cmd", cmd::getQueue());
    while (true) {
        teller::cmd::processNext();
    }
}

}
