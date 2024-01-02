#include <cassert>

#include "config.h"
#include "hal/led.h"

#include "stm32_hal.h"

using namespace teller::hal::led;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    bool inverted;
} led_config_t;

#if defined TELLER_BOARD_KAKUTEH7V2
// KakuteH7v2 blue LED, for testing purposes
const led_config_t led_config[NUM_LEDS] = {
    { GPIOC, GPIO_PIN_2, /* inverted = */ true }, /* Heartbeat LED */
    { 0 }, /* Error LED */
};
#elif defined STM32F1
// STM32VL-Discovery green led - PC9
const led_config_t led_config[NUM_LEDS] = {
    { GPIOC, GPIO_PIN_9 }, /* Heartbeat LED */
    { 0 }, /* Error LED */
};
#elif defined STM32H7
// STM32H743ZI blue LED
const led_config_t led_config[NUM_LEDS] = {
    { GPIOB, GPIO_PIN_7 }, /* Heartbeat LED */
    { 0 }, /* Error LED */
};
#elif defined STM32F4
// STM32F4-Discovery green led - PD12
const led_config_t led_config[NUM_LEDS] = {
    { GPIOD, GPIO_PIN_12 }, /* Heartbeat LED */
    { 0 }, /* Error LED */
};
#elif defined STM32L5
// NUCLEO-L552ZE-Q blue led - PB7
const led_config_t led_config[NUM_LEDS] = {
    { GPIOB, GPIO_PIN_7 }, /* Heartbeat LED */
    { 0 }, /* Error LED */
};
#else
// No LEDs supported on this hardware
const led_config_t led_config[NUM_LEDS] = {
    { 0 }, /* Heartbeat LED */
    { 0 }, /* Error LED */
};
#endif

static void configure_led(const led_config_t* cfg);

void teller::hal::led::init()
{
    for (size_t i = 0; i < NUM_LEDS; i++) {
        configure_led(&led_config[i]);
    }
}

void teller::hal::led::clear(led_t led)
{
    set(led, false);
}

void teller::hal::led::set(led_t led, bool value)
{
    assert(led >= 0 && led < NUM_LEDS);

    const led_config_t* cfg = &led_config[led];
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

#ifdef GPIOA
    if (cfg->port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
#endif

#ifdef GPIOB
    if (cfg->port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
#endif

#ifdef GPIOC
    if (cfg->port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
#endif

#ifdef GPIOD
    if (cfg->port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
#endif

#ifdef GPIOE
    if (cfg->port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
#endif

#ifdef GPIOF
    if (cfg->port == GPIOF) {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    }
#endif

#ifdef GPIOG
    if (cfg->port == GPIOG) {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    }
#endif

#ifdef GPIOH
    if (cfg->port == GPIOH) {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    }
#endif

    HAL_GPIO_Init(cfg->port, &GPIO_Config);
}
