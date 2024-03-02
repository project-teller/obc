#include <cassert>

#include "hal/gpio.h"

using namespace teller::hal::gpio;

/** State of simulated GPIO pins */
static uint8_t digitalPins = 0;

#define BIT(x) (1 << (x))

static_assert(GPIO_COUNT <= 8);

namespace teller::hal::gpio {

bool init()
{
    digitalPins = 0;
    return true;
}

void destroy()
{
    digitalPins = 0;
}

bool read(pin_t index)
{
    return digitalPins & BIT(index);
}

void write(pin_t index, bool value)
{
    if (value) {
        digitalPins |= BIT(index);
    } else {
        digitalPins &= ~BIT(index);
    }
}

}
