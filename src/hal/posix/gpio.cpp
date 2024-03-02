#include <cassert>
#include <cstring>

#include "hal/gpio.h"

using namespace teller::hal::gpio;

/** State of simulated digital GPIO pins */
static uint8_t digitalPins = 0;

/** State of simulated analog pins */
static uint16_t analogPins[AGPIO_COUNT];

#define BIT(x) (1 << (x))

static_assert(DGPIO_COUNT <= 8);

namespace teller::hal::gpio {

bool init()
{
    digitalPins = 0;
    memset(analogPins, 0, sizeof(analogPins));
    return true;
}

void destroy()
{
    digitalPins = 0;
    memset(analogPins, 0, sizeof(analogPins));
}

uint16_t readAnalog(analog_pin_t index)
{
    return index < AGPIO_COUNT ? analogPins[index] : 0;
}

bool readDigital(digital_pin_t index)
{
    return digitalPins & BIT(index);
}

void writeAnalog(analog_pin_t index, uint16_t value)
{
    if (index < AGPIO_COUNT) {
        analogPins[index] = value;
    }
}

void writeDigital(digital_pin_t index, bool value)
{
    if (value) {
        digitalPins |= BIT(index);
    } else {
        digitalPins &= ~BIT(index);
    }
}

}
