#pragma once

#include "stm32_hal.h"

#if defined(STM32H7)
#if defined(__ICCARM__)
#define DMA_BUFFER \
    _Pragma("location=\".dma_buffer\"")
#else
#define DMA_BUFFER \
    __attribute__((section(".dma_buffer")))
#endif
#else
#define DMA_BUFFER
#endif

namespace teller::hal::utils {

typedef struct {
    GPIO_TypeDef* port;
    uint32_t pins;
} gpio_port_and_pins_t;

void enableGPIOClocksForPort(const GPIO_TypeDef* port);

}
