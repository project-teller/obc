#include "hal/gpio.h"

#include "config.h"
#include "stm32_hal.h"
#include "utils.h"

using namespace teller::hal::gpio;
using namespace teller::hal::utils;

typedef struct {
    GPIO_TypeDef* port;
    uint32_t pin;
    GPIO_InitTypeDef init;
    uint8_t flags;
} gpio_config_t;

#define UNMAPPED   \
    {              \
        nullptr, 0 \
    }

#define INPUT_PIN                                                                  \
    {                                                                              \
        .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW \
    }
#define INPUT_PIN_PULLDOWN                                                           \
    {                                                                                \
        .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_FREQ_LOW \
    }
#define INPUT_PIN_PULLUP                                                           \
    {                                                                              \
        .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_FREQ_LOW \
    }
#define OUTPUT_PIN                                                                     \
    {                                                                                  \
        .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW \
    }

#define FLAG_INVERTED 1

const gpio_config_t gpio_configs[NUM_GPIO_PINS] = {
#if defined(TELLER_BOARD_NUCLEO144)
    // STM32H743ZI Nucleo-144 dev board, for testing purposes
    /* RXSM signals */
    /* - SODS: User button */
    { GPIOC, GPIO_PIN_13, INPUT_PIN },
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

    /* - Miscellaneous */
    UNMAPPED,
#elif defined(TELLER_BOARD_STM32F4)
    // STM32F415RG TELLER OBC v2.1

    /* RXSM signals */
    /* - SODS */
    { GPIOA, GPIO_PIN_3, INPUT_PIN, FLAG_INVERTED },
    /* - SOE */
    { GPIOB, GPIO_PIN_8, INPUT_PIN, FLAG_INVERTED },
    /* - LO */
    { GPIOD, GPIO_PIN_2, INPUT_PIN, FLAG_INVERTED },

    /* LCL signals */
    /* - STATUS_GMM_LCL */
    { GPIOA, GPIO_PIN_7, INPUT_PIN_PULLUP, FLAG_INVERTED },
    /* - STATUS_SCM_LCL */
    { GPIOC, GPIO_PIN_4, INPUT_PIN_PULLUP, FLAG_INVERTED },
    /* - STATUS_SUC_LCL1 */
    { GPIOB, GPIO_PIN_1, INPUT_PIN_PULLUP, FLAG_INVERTED },
    /* - STATUS_SUC_LCL2 */
    { GPIOB, GPIO_PIN_0, INPUT_PIN_PULLUP, FLAG_INVERTED },
    /* - STATUS_SUC_LCL3 */
    { GPIOC, GPIO_PIN_5, INPUT_PIN_PULLUP, FLAG_INVERTED },
    /* - STATUS_CAM_LCL */
    { GPIOC, GPIO_PIN_0, INPUT_PIN_PULLUP, FLAG_INVERTED },

    /* LCL reset */
    /* - RST_GMM_LCL */
    { GPIOA, GPIO_PIN_2, OUTPUT_PIN },
    /* - RST_SCM_LCL */
    { GPIOB, GPIO_PIN_11, OUTPUT_PIN },
    /* - RST_SUC_LCL1 */
    { GPIOA, GPIO_PIN_8, OUTPUT_PIN },
    /* - RST_SUC_LCL2 */
    { GPIOC, GPIO_PIN_9, OUTPUT_PIN },
    /* - RST_SUC_LCL3 */
    { GPIOC, GPIO_PIN_8, OUTPUT_PIN },
    /* - RST_CAM_LCL */
    { GPIOC, GPIO_PIN_1, OUTPUT_PIN },

    /* Miscellaneous */
    /* - START_CAM */
    { GPIOA, GPIO_PIN_5, OUTPUT_PIN }
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

    /* Miscellaneous */
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
            enableGPIOClocksForPort(cfg->port);

            init = cfg->init;
            init.Pin = cfg->pin;
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
        return HAL_GPIO_ReadPin(cfg->port, cfg->pin) == ((cfg->flags & FLAG_INVERTED) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    } else {
        return false;
    }
}

void write(pin_t index, bool value)
{
    const gpio_config_t* cfg = findPin(index);
    if (cfg) {
        if (cfg->flags & FLAG_INVERTED) {
            value = !value;
        }
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
