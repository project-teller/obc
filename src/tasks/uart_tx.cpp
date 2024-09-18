#include <cassert>

#include "modules/supervisor.h"
#include "modules/telem.h"
#include "tasks/uart_tx.h"

using namespace teller;

void teller::tasks::uartTxTask(void* arg)
{
    supervisor::QueueRegistration queue("uartTx", telem::getQueue());
    while (true) {
        telem::processNext();
    }
}
