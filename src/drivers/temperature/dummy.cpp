#include "config.h"
#include "drivers/temperature.h"

namespace teller::drivers::temperature {

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

bool update(float& temperature)
{
    temperature = 25.0f;
    return true;
}

}
