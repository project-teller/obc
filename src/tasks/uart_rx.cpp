#include "modules/uart_rx.h"
#include "hal/system.h"
#include "tasks/uart_rx.h"

namespace teller::tasks {

[[noreturn]] void uartRxTask(void* args_)
{
    uart_rx_task_args_t* args = static_cast<uart_rx_task_args_t*>(args_);
    if (isConnected(args->uart_index)) {
        while (true) {
            teller::uart_rx::read(args->uart_index);
        }
    } else {
        teller::hal::system::sleepForever();
    }
}

}
