#include <cassert>

#include "hal/gpio.h"
#include "hal/system.h"

using namespace teller::hal::gpio;

#define BIT(x) (1 << (x))

/** State of simulated GPIO pins */
static uint32_t digitalPins = 0;

/** Timestamps when GPIO pins were last set to 0 or 1 */
static uint32_t edgeTimestamps[NUM_GPIO_PINS * 2];

static const uint32_t digitalPinInitialValues = (
    /* Reset pins start in logical high, they must be pulled low to reset */
    BIT(RST_GMM_LCL) | BIT(RST_SCM_LCL) | BIT(RST_SUC_LCL1) | BIT(RST_SUC_LCL2) | BIT(RST_SUC_LCL3) | BIT(RST_HVPSU_LCL) |

    /* LCL status pins start in logical high */
    BIT(STATUS_GMM_LCL) | BIT(STATUS_SCM_LCL) | BIT(STATUS_SUC_LCL1) | BIT(STATUS_SUC_LCL2) | BIT(STATUS_SUC_LCL3) | BIT(STATUS_CAM_LCL));

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
    bool oldValue = read(index);
    uint8_t i;
    uint32_t* ts;

    if (value == oldValue) {
        return;
    }

    if (value) {
        digitalPins |= BIT(index);
    } else {
        digitalPins &= ~BIT(index);
    }

    i = value ? 1 : 0;
    ts = &edgeTimestamps[2 * index];
    ts[i] = teller::hal::system::getTimeSinceBootMsec() + 1;

    /* For LCL pins, process the pulses */
    if (value && ts[0] > 0 && ts[1] > 0) {
        if (index >= RST_GMM_LCL && index <= RST_HVPSU_LCL) {
            write(static_cast<pin_t>(index - (RST_GMM_LCL - STATUS_GMM_LCL)), 0);
        }
    }
}

}
