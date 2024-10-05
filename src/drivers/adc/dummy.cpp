#include "config.h"
#include "drivers/adc.h"

namespace teller::drivers::adc {

bool init()
{
    return true;
}

void destroy()
{
}

bool setup(void)
{
    return true;
}

bool update(std::uint8_t channel, float& value)
{
    value = 0.0f;
    return true;
}

}
