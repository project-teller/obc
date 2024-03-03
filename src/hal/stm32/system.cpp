#include <cassert>

#include "hal/system.h"

#include "stm32_hal.h"
#include <cmsis_os2.h>

using namespace teller::hal::system;

void teller::hal::system::init()
{
    SystemInit();

    assert(osKernelGetTickFreq() == 1000);
}

void teller::hal::system::destroy()
{
}

std::uint32_t teller::hal::system::getTimeSinceBootMsec(void)
{
    return osKernelGetTickCount();
}

void teller::hal::system::delayMsec(uint32_t delay)
{
    osDelay(delay);
}
