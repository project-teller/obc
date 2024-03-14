#include "modules/cmd.h"
#include "tasks/cmd.h"

[[noreturn]] void teller::tasks::commandTask(void* arg)
{
    while (true) {
        teller::cmd::handleNext();
    }
}
