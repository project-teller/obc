#include <cassert>

#include "hal/system.h"

#include "stm32_hal.h"
#include <cmsis_os2.h>

using namespace teller::hal::system;

namespace teller::hal::system {

void init()
{
    HAL_Init();

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

void sleepForever()
{
    while (true) {
        osDelay(osWaitForever);
    }
}

void sleepUntilMsec(uint32_t deadline)
{
    uint32_t now = getTimeSinceBootMsec();
    if (now < deadline) {
        osDelay(deadline - now);
    } else {
        osThreadYield();
    }
}

void yield(void)
{
    osThreadYield();
}

}

extern "C" {

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
#ifdef STM32F4
    __HAL_RCC_PWR_CLK_ENABLE();
#endif

    /* System interrupt init*/
    /* PendSV_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
}
}
