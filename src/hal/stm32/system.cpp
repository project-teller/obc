#include "hal/system.h"

#include "stm32_hal.h"

void teller::hal::system::init()
{
    SystemInit();
}
