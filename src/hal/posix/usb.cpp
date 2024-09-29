#include "hal/usb.h"

namespace teller::hal::usb {

bool init()
{
    return true;
}

void destroy()
{
}

/* TODO: move the handling of the USB debug port via sockets here */
bool isConnected()
{
    return false;
}

}
