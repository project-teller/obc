#include "tasks/debug.h"

#include "hal/uart.h"
#include "modules/mode.h"
#include "modules/telem.h"

#include <cstdio>

using namespace teller::hal::uart;
using namespace teller::mode;
using namespace teller::telem;

[[noreturn]] void teller::tasks::debugTask(void* arg)
{
    while (true) {
        waitUntilConnected(DEBUG);
        notifyPossibleModeChange(MODE_CHANGE_REASON_DEBUG_UART);

        requestTelemetry(DEBUG);

        waitUntilDisconnected(DEBUG);
        notifyPossibleModeChange(MODE_CHANGE_REASON_DEBUG_UART);

        stopTelemetry(DEBUG);
    }
}
