#include "modules/uart_rx.h"
#include "hal/system.h"
#include "tasks/uart_rx.h"

namespace teller::tasks {

[[noreturn]] void uartRxTask(void* args_)
{
    uart_rx_task_args_t* args = static_cast<uart_rx_task_args_t*>(args_);

    while (true) {
        waitUntilConnected(args->uart_index);
        while (isConnected(args->uart_index)) {
            teller::uart_rx::read(args->uart_index);
        }
    }
}

}
