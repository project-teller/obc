#include "utils.h"

using namespace teller::hal::utils;

namespace teller::hal::utils {

void enable_gpio_clocks_for_port(const GPIO_TypeDef* port)
{
#ifdef GPIOA
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
#endif

#ifdef GPIOB
    if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
#endif

#ifdef GPIOC
    if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
#endif

#ifdef GPIOD
    if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
#endif

#ifdef GPIOE
    if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
#endif

#ifdef GPIOF
    if (port == GPIOF) {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    }
#endif

#ifdef GPIOG
    if (port == GPIOG) {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    }
#endif

#ifdef GPIOH
    if (port == GPIOH) {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    }
#endif
}

}
