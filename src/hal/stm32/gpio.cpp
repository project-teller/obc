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

const gpio_config_t gpio_configs[NUM_GPIO_PINS] = {
#if defined(TELLER_BOARD_NUCLEO144)
    // STM32H743ZI Nucleo-144 dev board, for testing purposes
    /* RXSM signals */
    /* - SODS: User button */
    { GPIOC, GPIO_PIN_13, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_FREQ_LOW } },
    UNMAPPED,
    UNMAPPED,

    /* LCL signals */
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,

    /* LCL reset */
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,

#elif defined(TELLER_BOARD_STM32F4)
    // STM32F415RG TELLER OBC

    /* RXSM signals */
    /* - SODS */
    { GPIOA, GPIO_PIN_3, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - SOE */
    { GPIOB, GPIO_PIN_8, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - LO */
    { GPIOD, GPIO_PIN_2, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },

    /* LCL signals */
    /* - STATUS_GMM_LCL */
    { GPIOA, GPIO_PIN_7, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - STATUS_SCM_LCL */
    { GPIOC, GPIO_PIN_4, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - STATUS_SUC_LCL1 */
    { GPIOB, GPIO_PIN_1, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - STATUS_SUC_LCL2 */
    { GPIOB, GPIO_PIN_0, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - STATUS_SUC_LCL3 */
    { GPIOC, GPIO_PIN_5, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - STATUS_CAM_LCL */
    { GPIOC, GPIO_PIN_8, { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },

    /* LCL reset */
    /* - RST_GMM_LCL */
    { GPIOA, GPIO_PIN_13, { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - RST_SCM_LCL */
    { GPIOA, GPIO_PIN_14, { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - RST_SUC_LCL1 */
    { GPIOA, GPIO_PIN_10, { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - RST_SUC_LCL2 */
    { GPIOC, GPIO_PIN_9, { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - RST_SUC_LCL3 */
    { GPIOC, GPIO_PIN_8, { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
    /* - RST_CAM_LCL */
    { GPIOC, GPIO_PIN_1, { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW } },
#else
    // No GPIOs supported on this hardware

    /* RXSM signals */
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,

    /* LCL signals */
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,

    /* LCL reset */
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
    UNMAPPED,
#endif
};

static const gpio_config_t* findPin(pin_t index);

namespace teller::hal::gpio {

bool init()
{
    GPIO_InitTypeDef init;

    for (size_t i = 0; i < NUM_GPIO_PINS; i++) {
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
    for (size_t i = 0; i < NUM_GPIO_PINS; i++) {
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

    if (index >= 0 && index < NUM_GPIO_PINS) {
        result = &gpio_configs[index];
    }

    return (result && result->port) ? result : nullptr;
}
