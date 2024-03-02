#include "hal/gpio.h"

#include "stm32_hal.h"

using namespace teller::hal::gpio;
using namespace teller::rxsm;

namespace teller::hal::gpio {

bool init()
{
    return true;
}

void destroy()
{
}

uint16_t readAnalog(analog_pin_t index)
{
    /* TODO */
    return 0;
}

bool readDigital(digital_pin_t index)
{
    /* TODO */
    return false;
}

void writeAnalog(analog_pin_t index, uint16_t value)
{
    /* TODO */
}

void writeDigital(digital_pin_t index, bool value)
{
    /* TODO */
}

}
