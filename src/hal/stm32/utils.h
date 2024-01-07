#pragma once

#include "stm32_hal.h"

namespace teller::hal::utils {

void enable_gpio_clocks_for_port(const GPIO_TypeDef* port);

}
