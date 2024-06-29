#pragma once

#include "hal/uart.h"

namespace teller::tasks {

typedef struct {
    teller::hal::uart::uart_t uart_index;
} cmd_task_args_t;

[[noreturn]] void commandTask(void* args);

}
