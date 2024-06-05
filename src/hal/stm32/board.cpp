#include "hal/board.h"

#include "stm32_hal.h"

using namespace teller::hal::board;

static reset_reason_t reasonOfLastReset = RESET_REASON_UNKNOWN;

namespace teller::hal::board {

bool init()
{
    RCC_OscInitTypeDef oscInit;
    RCC_ClkInitTypeDef clkInit;

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

    /*
    oscInit.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    oscInit.HSIState = RCC_HSI_DIV1;
    oscInit.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscInit.LSIState = RCC_LSI_ON;
    oscInit.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&oscInit) != HAL_OK) {
        return false;
    }
    */

    /*
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
    */

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
