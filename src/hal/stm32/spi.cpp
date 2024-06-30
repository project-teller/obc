#include "stm32_hal.h"

#include "hal/spi.h"
#include "hal/stm32/utils.h"

using namespace teller::hal::utils;

#define NUM_GPIO_CFG 4

typedef struct {
    SPI_TypeDef* instance;
    gpio_port_and_pins_t gpio[NUM_GPIO_CFG];

    uint32_t baud_rate;
} spi_config_t;

typedef struct {
    SPI_HandleTypeDef handle;
    const spi_config_t* cfg;
    bool initialized;
} spi_state_t;

#define NO_MORE_SPI_BUSES \
    {                     \
        0                 \
    }

#define NO_MORE_GPIO_CFG \
    {                    \
        0                \
    }

#if defined TELLER_BOARD_NUCLEO144
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#define NUM_SPI_BUSES 2
const spi_config_t spi_config[] = {
    { .instance = SPI1,
        .gpio_cfg = {
            { GPIOA, GPIO_PIN_5 | GPIO_PIN_6 },
            { GPIOD, GPIO_PIN_7 },
            NO_MORE_GPIO_CFG } },
    { .instance = SPI2, .gpio_cfg = { { GPIOC, GPIO_PIN_2 | GPIO_PIN_3 }, { GPIOB, GPIO_PIN_10 }, NO_MORE_GPIO_CFG } }, NO_MORE_SPI_BUSES
};
#elif defined STM32F4
// STM32F4-Discovery
#define NUM_SPI_BUSES 0
const spi_config_t spi_config[] = {
    NO_MORE_SPI_BUSES
};
#else
// No SPI buses supported on this hardware
#define NUM_SPI_BUSES 0
const spi_config_t spi_config[] = {
    NO_MORE_SPI_BUSES
};
#endif

static bool configure_spi_bus(spi_state_t* state, const spi_config_t* cfg);
static spi_state_t* find_spi_state(SPI_HandleTypeDef* hspi);

static SPI_HandleTypeDef* spi_handle_ptrs[7];
static spi_state_t spi_state[NUM_SPI_BUSES];

namespace teller::hal::spi {

bool init()
{
    for (size_t i = 0; i < NUM_SPI_BUSES; i++) {
        if (!configure_spi_bus(&spi_state[i], &spi_config[i])) {
            return false;
        }
    }

    return true;
}

void destroy()
{
}

}

/* ************************************************************************** */

static bool configure_spi_bus(spi_state_t* state, const spi_config_t* cfg)
{
    bool success = false;

    if (state->initialized || (state->cfg && state->cfg != cfg)) {
        return false;
    }

    state->cfg = cfg;

    if (cfg->instance == nullptr) {
        return true;
    }

    SPI_HandleTypeDef* pHandle = &state->handle;

    pHandle->Instance = cfg->instance;

    pHandle->Init.Mode = SPI_MODE_MASTER;
    pHandle->Init.Direction = SPI_DIRECTION_2LINES;
    pHandle->Init.DataSize = SPI_DATASIZE_8BIT;
    pHandle->Init.CLKPolarity = SPI_POLARITY_LOW;
    pHandle->Init.CLKPhase = SPI_PHASE_1EDGE;
    pHandle->Init.NSS = SPI_NSS_SOFT;
    pHandle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    pHandle->Init.FirstBit = SPI_FIRSTBIT_MSB;
    pHandle->Init.TIMode = SPI_TIMODE_DISABLE;
    pHandle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    pHandle->Init.CRCPolynomial = 0x0;
    pHandle->Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    pHandle->Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    pHandle->Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    pHandle->Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    pHandle->Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    pHandle->Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    pHandle->Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    pHandle->Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    pHandle->Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    pHandle->Init.IOSwap = SPI_IO_SWAP_DISABLE;

    if (HAL_SPI_Init(pHandle) != HAL_OK) {
        goto cleanup;
    }

    /* HAL_SPI_MspInit sets state->initialized to true. If this is still false
     * at this point, it means that the initialization was not successful */
    if (!state->initialized) {
        goto cleanup;
    }

    success = true;

cleanup:
    if (state->initialized && !success) {
        HAL_SPI_DeInit(pHandle);
        state->initialized = false;
    }

    return success;
}

/* Finds the SPI configuration corresponding to the given physical SPI bus handle */
static spi_state_t* find_spi_state(SPI_HandleTypeDef* hspi)
{
    for (int index = 0; index < NUM_SPI_BUSES; index++) {
        if (hspi == &spi_state[index].handle) {
            return &spi_state[index];
        }
    }

    return nullptr;
}

/* ************************************************************************** */

extern "C" {

/* Weakly linked function that is called by the STM32 HAL when a physical SPI bus is
 * initialized
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    uint8_t gpioFunc = 0;

#ifdef STM32H7
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
    PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
#endif

    spi_state_t* state = find_spi_state(hspi);
    const spi_config_t* cfg = state ? state->cfg : nullptr;
    if (!cfg) {
        return;
    }

    if (hspi->Instance == SPI1) {
        gpioFunc = GPIO_AF5_SPI1;

#ifdef STM32H7
        /* Initializes the peripherals clock */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            return;
        }
#endif

        /* Peripheral clock enable */
        __HAL_RCC_SPI1_CLK_ENABLE();
    } else if (hspi->Instance == SPI2) {
        gpioFunc = GPIO_AF5_SPI2;

#ifdef STM32H7
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

        /* Initializes the peripherals clock */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            return;
        }
#endif

        /* Peripheral clock enable */
        __HAL_RCC_SPI2_CLK_ENABLE();
    }

    /* GPIO configuration */
    for (int i = 0; i < NUM_GPIO_CFG; i++) {
        if (cfg->gpio[i].port && cfg->gpio[i].pins) {
            enableGPIOClocksForPort(cfg->gpio[i].port);
            GPIO_InitStruct.Pin = cfg->gpio[i].pins;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            GPIO_InitStruct.Alternate = gpioFunc;
            HAL_GPIO_Init(cfg->gpio[i].port, &GPIO_InitStruct);
        }
    }

    state->initialized = true;

    if (hspi->Instance == SPI1) {
        spi_handle_ptrs[1] = hspi;
    } else if (hspi->Instance == SPI2) {
        spi_handle_ptrs[2] = hspi;
    }
}

/* Weakly linked function that is called by the STM32 HAL when an SPI bus is
 * deinitialized
 */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
    spi_state_t* state = find_spi_state(hspi);
    const spi_config_t* cfg = state ? state->cfg : nullptr;
    if (!cfg) {
        return;
    }

    if (hspi->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_DISABLE();
    } else if (hspi->Instance == SPI2) {
        __HAL_RCC_SPI2_CLK_DISABLE();
    }

    for (int i = NUM_GPIO_CFG - 1; i >= 0; i--) {
        if (cfg->gpio[i].port && cfg->gpio[i].pins) {
            HAL_GPIO_DeInit(cfg->gpio[i].port, cfg->gpio[i].pins);
        }
    }

    if (state) {
        state->initialized = false;
    }

    if (hspi->Instance == SPI1) {
        spi_handle_ptrs[1] = nullptr;
    } else if (hspi->Instance == SPI2) {
        spi_handle_ptrs[2] = nullptr;
    }
}
}
