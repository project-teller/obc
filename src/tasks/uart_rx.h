#pragma once

#include "hal/uart.h"

namespace teller::tasks {

typedef struct {
    const char* task_name;
    teller::hal::uart::uart_t uart_index;
} uart_rx_task_args_t;

[[noreturn]] void uartRxTask(void* args);

}
