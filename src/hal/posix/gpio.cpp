#include <cassert>

#include "hal/gpio.h"

using namespace teller::hal::gpio;

#define BIT(x) (1 << (x))

/** State of simulated GPIO pins */
static uint32_t digitalPins = 0;

static const uint32_t digitalPinInitialValues = (
    /* Reset pins start in logical high, they must be pulled low to reset */
    BIT(RST_GMM_LCL) | BIT(RST_SCM_LCL) | BIT(RST_SUC_LCL1) | BIT(RST_SUC_LCL2) | BIT(RST_SUC_LCL3) | BIT(RST_HVPSU_LCL) |

    /* LCL status pins start in logical high */
    BIT(STATUS_GMM_LCL) | BIT(STATUS_SCM_LCL) | BIT(STATUS_SUC_LCL1) | BIT(STATUS_SUC_LCL2) | BIT(STATUS_SUC_LCL3) | BIT(STATUS_HVPSU_LCL));

static_assert(NUM_GPIO_PINS <= 32);

namespace teller::hal::gpio {

bool init()
{
    digitalPins = digitalPinInitialValues;
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
