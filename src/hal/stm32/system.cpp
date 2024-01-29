#include <cassert>

#include "hal/system.h"

#include "stm32_hal.h"
#include <cmsis_os2.h>

using namespace teller::hal::system;

reset_reason_t reasonOfLastReset = RESET_REASON_UNKNOWN;

void teller::hal::system::init()
{
    SystemInit();

    assert(osKernelGetTickFreq() == 1000);

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDG1RST)) {
        reasonOfLastReset = RESET_REASON_WATCHDOG;
    } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
        reasonOfLastReset = RESET_REASON_SOFTWARE;
    } else {
        reasonOfLastReset = RESET_REASON_NORMAL;
    }

    __HAL_RCC_CLEAR_RESET_FLAGS();
}

void teller::hal::system::destroy()
{
}

reset_reason_t teller::hal::system::getReasonOfLastReset(void)
{
    return reasonOfLastReset;
}

std::uint32_t teller::hal::system::getTimeSinceBootMsec(void)
{
    return osKernelGetTickCount();
}
