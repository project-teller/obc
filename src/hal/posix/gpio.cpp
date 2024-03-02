#include <cassert>

#include "hal/gpio.h"

using namespace teller::hal::gpio;

static uint8_t gpioPins = 0;

namespace teller::hal::gpio {

bool init()
{
    gpioPins = 0;
    return true;
}

void destroy()
{
    gpioPins = 0;
}

}
