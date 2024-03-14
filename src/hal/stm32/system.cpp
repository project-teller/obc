#include <cassert>

#include "hal/system.h"

#include "stm32_hal.h"
#include <cmsis_os2.h>

using namespace teller::hal::system;

namespace teller::hal::system {

void init()
{
    SystemInit();

    assert(osKernelGetTickFreq() == 1000);
}

void destroy()
{
}

std::uint32_t getTimeSinceBootMsec(void)
{
    return osKernelGetTickCount();
}

void delayMsec(uint32_t delay)
{
    osDelay(delay);
}

void requestReset(void)
{
    HAL_NVIC_SystemReset();
}

}
