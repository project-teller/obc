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

bool update(std::uint8_t count, float* value)
{
    for (std::uint8_t i = 0; i < count; i++) {
        value[i] = 0.0f;
    }
    return true;
}

}
