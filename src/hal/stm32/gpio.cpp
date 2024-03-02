#include "hal/gpio.h"

#include "stm32_hal.h"

using namespace teller::hal::gpio;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} gpio_config_t;

#define UNMAPPED   \
    {              \
        nullptr, 0 \
    }

const gpio_config_t gpio_configs[GPIO_COUNT] = {
#if defined(TELLER_BOARD_NUCLEO144)
    UNMAPPED,
    UNMAPPED,
    UNMAPPED
#elif defined(STM32F4)
    // STM32F4-Discovery
    UNMAPPED,
    UNMAPPED,
    UNMAPPED
#else
    // No UART supported on this hardware
    UNMAPPED,
    UNMAPPED,
    UNMAPPED
#endif
};

static const gpio_config_t* findPin(pin_t index);

namespace teller::hal::gpio {

bool init()
{
    return true;
}

void destroy()
{
}

bool read(pin_t index)
{
    const gpio_config_t* cfg = findPin(index);
    if (cfg) {
        return HAL_GPIO_ReadPin(cfg->port, cfg->pin) == GPIO_PIN_SET;
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
