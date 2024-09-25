#include "modules/lcl.h"

#include "hal/gpio.h"
#include "hal/system.h"

using namespace teller::hal::gpio;

/**
 * @brief Pair of GPIO pins associated to an LCL.
 *
 * Each LCL has two pins: a query pin and a reset pin.
 */
typedef struct {
    pin_t query_pin;
    pin_t reset_pin;
} lcl_pins_t;

/** Pulse duration to use for resetting an LCL. Used to speed up unit tests. */
static uint16_t pulse_duration_msec;

namespace teller::lcl {

/**
 * @brief Table mapping LCL indices to their GPIO pins.
 */
static const lcl_pins_t pin_map[NUM_LCLS] = {
    { STATUS_GMM_LCL, RST_GMM_LCL },
    { STATUS_SCM_LCL, RST_SCM_LCL },
    { STATUS_SUC_LCL1, RST_SUC_LCL1 },
    { STATUS_SUC_LCL2, RST_SUC_LCL2 },
    { STATUS_SUC_LCL3, RST_SUC_LCL3 },
    { STATUS_CAM_LCL, RST_CAM_LCL },
};

bool init()
{
    pulse_duration_msec = 100;

    /* LCL reset pins start from logical high */
    for (int i = 0; i < NUM_LCLS; i++) {
        pin_t pin = pin_map[static_cast<lcl_t>(i)].reset_pin;
        write(pin, 1);
    }

    return true;
}

void destroy()
{
}

bool triggered(lcl_t lcl)
{
    if (lcl >= 0 && lcl < NUM_LCLS) {
        return read(pin_map[lcl].query_pin);
    } else {
        return false;
    }
}

void reset(lcl_t lcl)
{
    if (lcl >= 0 && lcl < NUM_LCLS) {
        pin_t pin = pin_map[lcl].reset_pin;
        write(pin, 0);
        teller::hal::system::delayMsec(pulse_duration_msec);
        write(pin, 1);
    }
}

void setResetPulseDurationMsec(uint16_t duration_msec)
{
    pulse_duration_msec = duration_msec;
}

}
