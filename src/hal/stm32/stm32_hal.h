#pragma once

#if defined STM32F1
#include <stm32f1xx_hal.h>
#elif defined STM32H7
#include <stm32h7xx_hal.h>
#elif defined STM32F4
#include <stm32f4xx_hal.h>
#elif defined STM32L5
#include <stm32l5xx_hal.h>
#endif
