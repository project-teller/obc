#include "tasks/usb.h"
#include "hal/system.h"
#include "hal/usb.h"
#include "modules/errors.h"

using namespace teller::errors;
using namespace teller::hal;

[[noreturn]] void teller::tasks::usbTask(void* arg)
{
    if (!usb::setup()) {
        setError(SYSTEM_INIT_ERROR);
    } else {
    }

    system::sleepForever();
}
