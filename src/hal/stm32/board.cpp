#include "hal/board.h"

#include "stm32_hal.h"

using namespace teller::hal::board;

static reset_reason_t reasonOfLastReset = RESET_REASON_UNKNOWN;

namespace teller::hal::board {

bool init()
{
    RCC_OscInitTypeDef oscInit = { 0 };
    RCC_ClkInitTypeDef clkInit = { 0 };

#if defined(STM32H7)
#define RCC_FLAG_IWDGRST RCC_FLAG_IWDG1RST
#endif

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        reasonOfLastReset = RESET_REASON_WATCHDOG;
    } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
        reasonOfLastReset = RESET_REASON_SOFTWARE;
    } else {
        reasonOfLastReset = RESET_REASON_NORMAL;
    }

    __HAL_RCC_CLEAR_RESET_FLAGS();

    /* Configure the oscillators */
    oscInit.OscillatorType = (RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE);

    /* HSE: High Speed External oscillator */
    oscInit.HSEState = RCC_HSE_BYPASS;

    /* HSI: High Speed Internal oscillator (64 MHz @ STM32H7) */
    oscInit.HSIState = RCC_HSI_DIV1;
    oscInit.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

    /* LSI: Low Speed Internal oscillator (32 kHz @ STM32H7) */
    oscInit.LSIState = RCC_LSI_ON;

    oscInit.PLL.PLLState = RCC_PLL_ON;
    oscInit.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscInit.PLL.PLLM = 1;
    oscInit.PLL.PLLN = 24;
    oscInit.PLL.PLLP = 2; /* Division factor for system clock */
    oscInit.PLL.PLLQ = 4; /* Division factor for peripheral clock */
    oscInit.PLL.PLLR = 2; /* Division factor for peripheral clock */
    oscInit.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    oscInit.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    oscInit.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&oscInit) != HAL_OK) {
        return false;
    }

    clkInit.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
        | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    clkInit.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clkInit.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clkInit.AHBCLKDivider = RCC_HCLK_DIV1;
    clkInit.APB3CLKDivider = RCC_APB3_DIV1;
    clkInit.APB1CLKDivider = RCC_APB1_DIV1;
    clkInit.APB2CLKDivider = RCC_APB2_DIV1;
    clkInit.APB4CLKDivider = RCC_APB4_DIV1;

    if (HAL_RCC_ClockConfig(&clkInit, FLASH_LATENCY_1) != HAL_OK) {
        return false;
    }

    return true;
}

void destroy()
{
}

float getBoardTemperature()
{
    return 0.0f;
}

float getBoardVoltage()
{
    return 0.0f;
}

reset_reason_t getReasonOfLastReset(void)
{
    return reasonOfLastReset;
}

}

extern "C" {

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM1) {
        HAL_IncTick();
    }
}
}
