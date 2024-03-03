#include "hal/gpio.h"

#include "config.h"
#include "stm32_hal.h"
#include "utils.h"

using namespace teller::hal::gpio;
using namespace teller::hal::utils;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    GPIO_InitTypeDef init;
} gpio_config_t;

#define UNMAPPED   \
    {              \
        nullptr, 0 \
    }

const gpio_config_t gpio_configs[GPIO_COUNT] = {
#if defined(TELLER_BOARD_NUCLEO144)
    /* SODS: User button */
    { GPIOC, GPIO_PIN_13, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_FREQ_HIGH } },
    UNMAPPED,
    UNMAPPED
#elif defined(STM32F4)
    // STM32F4-Discovery
    UNMAPPED,
    UNMAPPED,
    UNMAPPED
#else
    // No GPIOs supported on this hardware
    UNMAPPED,
    UNMAPPED,
    UNMAPPED
#endif
};

static const gpio_config_t* findPin(pin_t index);

namespace teller::hal::gpio {

bool init()
{
    GPIO_InitTypeDef init;

    for (size_t i = 0; i < GPIO_COUNT; i++) {
        const gpio_config_t* cfg = &gpio_configs[i];
        if (cfg->port) {
            init = cfg->init;
            init.Pin = cfg->pin;
            enableGPIOClocksForPort(cfg->port);
            HAL_GPIO_Init(cfg->port, &init);
        }
    }

    return true;
}

void destroy()
{
    for (size_t i = 0; i < GPIO_COUNT; i++) {
        const gpio_config_t* cfg = &gpio_configs[i];
        if (cfg->port) {
            HAL_GPIO_DeInit(cfg->port, cfg->pin);
        }
    }
}

bool read(pin_t index)
{
    const gpio_config_t* cfg = findPin(index);
    if (cfg) {
        return HAL_GPIO_ReadPin(cfg->port, cfg->pin) != GPIO_PIN_RESET;
    } else {
        return false;
    }
}

void write(pin_t index, bool value)
{
    const gpio_config_t* cfg = findPin(index);
    if (cfg) {
        HAL_GPIO_WritePin(cfg->port, cfg->pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    };
}

}

static const gpio_config_t* findPin(pin_t index)
{
    const gpio_config_t* result = nullptr;

    if (index >= 0 && index < GPIO_COUNT) {
        result = &gpio_configs[index];
    }

    return (result && result->port) ? result : nullptr;
}
