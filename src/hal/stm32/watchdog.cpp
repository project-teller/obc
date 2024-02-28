#include "hal/watchdog.h"

#include "stm32_hal.h"

IWDG_HandleTypeDef wdg;

bool teller::hal::watchdog::init()
{
    /* TODO: check whether the previous reset was triggered by the watchdog */
    return true;
}

void teller::hal::watchdog::configure_and_start()
{
    /* Watchdog clock runs at 32 kHz. Setting the prescaler to 32 will make the
     * watchdog counter count down by 1000/s so we have a granularity of 1 msec */

#if defined(STM32F4)
    wdg.Instance = IWDG;
    wdg.Init.Prescaler = IWDG_PRESCALER_32;
    wdg.Init.Reload = 3000; /* ticks, but also msec */
#elif defined(STM32H7)
    wdg.Instance = IWDG1;
    wdg.Init.Prescaler = IWDG_PRESCALER_32;
    wdg.Init.Reload = 3000; /* ticks, but also msec */
    wdg.Init.Window = 3000;
#else
#error "Watchdog support not implemented for this STM32 variant"
#endif

    HAL_IWDG_Init(&wdg);
}

void teller::hal::watchdog::reset()
{
    HAL_IWDG_Refresh(&wdg);
}
