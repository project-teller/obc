#pragma once

#include "stm32_hal.h"

namespace teller::hal::utils {

void enableGPIOClocksForPort(const GPIO_TypeDef* port);

}
