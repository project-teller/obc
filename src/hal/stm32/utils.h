#pragma once

#include "stm32_hal.h"

namespace teller::hal::utils {

typedef struct {
    GPIO_TypeDef* port;
    uint32_t pins;
} gpio_port_and_pins_t;

void enableGPIOClocksForPort(const GPIO_TypeDef* port);

}
