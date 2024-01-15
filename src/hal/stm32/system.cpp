#include "hal/system.h"

#include "stm32_hal.h"

using namespace teller::hal::system;

reset_reason_t reasonOfLastReset = RESET_REASON_UNKNOWN;

void teller::hal::system::init()
{
    SystemInit();

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDG1RST)) {
        reasonOfLastReset = RESET_REASON_WATCHDOG;
    } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
        reasonOfLastReset = RESET_REASON_SOFTWARE;
    } else {
        reasonOfLastReset = RESET_REASON_NORMAL;
    }

    __HAL_RCC_CLEAR_RESET_FLAGS();
}

reset_reason_t teller::hal::system::getReasonOfLastReset(void)
{
    return reasonOfLastReset;
}
