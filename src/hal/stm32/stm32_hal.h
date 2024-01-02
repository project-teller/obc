#pragma once

#if defined STM32F1
#include <stm32f1xx_hal.h>

// STM32VL-Discovery green led - PC9
#define LED_PORT GPIOC
#define LED_PIN GPIO_PIN_9
// STM32VL-Discovery blue led - PC8
// #define LED_PIN                 GPIO_PIN_8
#define LED_PORT_CLK_ENABLE __HAL_RCC_GPIOC_CLK_ENABLE
#elif defined STM32H7
#include <stm32h7xx_hal.h>

// STM32H743ZI blue LED
#define LED_PORT GPIOB
#define LED_PIN GPIO_PIN_7
#define LED_PORT_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE
#elif defined STM32F4
#include <stm32f4xx_hal.h>

// STM32F4-Discovery green led - PD12
#define LED_PORT GPIOD
#define LED_PIN GPIO_PIN_12
#define LED_PORT_CLK_ENABLE __HAL_RCC_GPIOD_CLK_ENABLE
#elif defined STM32L5
#include <stm32l5xx_hal.h>

// NUCLEO-L552ZE-Q blue led - PB7
#define LED_PORT GPIOB
#define LED_PIN GPIO_PIN_7
#define LED_PORT_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE
#endif
