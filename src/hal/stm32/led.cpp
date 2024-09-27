#include <cassert>

#include "config.h"
#include "hal/led.h"

#include "stm32_hal.h"
#include "utils.h"

using namespace teller::hal::led;
using namespace teller::hal::utils;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    bool inverted;
} led_config_t;

#if defined TELLER_BOARD_NUCLEO144
// STM32H743ZI Nucleo-144 dev board, for testing purposes
const led_config_t led_config[NUM_LEDS] = {
    { GPIOB, GPIO_PIN_0 }, /* Heartbeat LED */
    { GPIOB, GPIO_PIN_14 }, /* Error LED */
    { GPIOE, GPIO_PIN_1 }, /* Debug LED */
};
#elif defined TELLER_BOARD_STM32F4
// STM32F415RG TELLER OBC
const led_config_t led_config[NUM_LEDS] = {
    { GPIOA, GPIO_PIN_6 }, /* Heartbeat LED */
    { 0 }, /* Error LED */
    { 0 }, /* Debug LED */
};
#else
// No LEDs supported on this hardware
const led_config_t led_config[NUM_LEDS] = {
    { 0 }, /* Heartbeat LED */
    { 0 }, /* Error LED */
    { 0 }, /* Debug LED */
};
#endif

static void configure_led(const led_config_t* cfg);
static const led_config_t* find_config(led_t led);

void teller::hal::led::init()
{
    for (size_t i = 0; i < NUM_LEDS; i++) {
        configure_led(&led_config[i]);
    }
}

void teller::hal::led::destroy()
{
}

void teller::hal::led::clear(led_t led)
{
    set(led, false);
}

bool teller::hal::led::has(led_t led)
{
    return find_config(led)->port != nullptr;
}

bool teller::hal::led::get(led_t led)
{
    const led_config_t* cfg = find_config(led);
    return cfg->port != nullptr && HAL_GPIO_ReadPin(cfg->port, cfg->pin) == GPIO_PIN_SET;
}

void teller::hal::led::set(led_t led, bool value)
{
    const led_config_t* cfg = find_config(led);
    if (cfg->port != nullptr) {
        HAL_GPIO_WritePin(
            cfg->port, cfg->pin,
            value ^ cfg->inverted ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

/* ************************************************************************** */

static void configure_led(const led_config_t* cfg)
{
    GPIO_InitTypeDef GPIO_Config;

    if (cfg == nullptr) {
        return;
    }

    GPIO_Config.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Config.Pull = GPIO_NOPULL;
    GPIO_Config.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_Config.Pin = cfg->pin;

    enableGPIOClocksForPort(cfg->port);

    HAL_GPIO_Init(cfg->port, &GPIO_Config);
}

static const led_config_t* find_config(led_t led)
{
    assert(led >= 0 && led < NUM_LEDS);
    return &led_config[led];
}
