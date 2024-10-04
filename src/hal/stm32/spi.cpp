#include "config.h"
#include "stm32_hal.h"
#include <cmsis_os2.h>

#include <limits>

#include "hal/mutex.hpp"
#include "hal/spi.h"
#include "hal/stm32/utils.h"
#include "hal/system.h"

using namespace teller::hal::utils;

#define SPI_TIMEOUT_MSEC 100

static const uint32_t EVT_DONE = 0x00000001U;
static const uint32_t EVT_ERROR = 0x00000002U;

#define NUM_GPIO_PINS_PER_BUS 3
#define NUM_CS_PINS_PER_BUS 6

typedef struct {
    SPI_TypeDef* instance;
    union {
        gpio_port_and_pins_t by_index[NUM_GPIO_PINS_PER_BUS];
        struct {
            gpio_port_and_pins_t clk;
            gpio_port_and_pins_t miso;
            gpio_port_and_pins_t mosi;
        } by_name;
    } gpio;
    gpio_port_and_pins_t cs[NUM_CS_PINS_PER_BUS];
    uint8_t mode;
    IRQn_Type irq;

    uint32_t baud_rate;
} spi_bus_config_t;

typedef struct {
    SPI_HandleTypeDef handle;
    const spi_bus_config_t* cfg;
    osEventFlagsId_t event;
    bool initialized;
    teller::hal::mutex in_use;
} spi_bus_state_t;

#define NO_MORE_SPI_BUSES \
    {                     \
        0                 \
    }

#define NO_MORE_GPIO_CFG \
    {                    \
        0                \
    }

/* clang-format off */
#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#define NUM_SPI_BUSES 2
const spi_bus_config_t spi_config[] = {
    {
        .instance = SPI1,
        .gpio = {
            .by_name = {
                .clk = { GPIOA, GPIO_PIN_5 },
                .miso = { GPIOA, GPIO_PIN_6 },
                .mosi = { GPIOB, GPIO_PIN_5 }
            }
        },
        .cs = {
            // Device 0
            { GPIOD, GPIO_PIN_14 },
            NO_MORE_GPIO_CFG
        },
        .mode = 0,
        .irq = SPI1_IRQn,
    },
    NO_MORE_SPI_BUSES
};
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board
#define NUM_SPI_BUSES 3
const spi_bus_config_t spi_config[] = {
    {
        .instance = SPI1,
        .gpio = {
            .by_name = {
                .clk = { GPIOB, GPIO_PIN_3 },
                .miso = { GPIOB, GPIO_PIN_4 },
                .mosi = { GPIOB, GPIO_PIN_5 }
            }
        },
        .cs = {
            // Device 0
            { GPIOA, GPIO_PIN_15 },
            NO_MORE_GPIO_CFG
        },
        .mode = 0,
        .irq = SPI1_IRQn,
    },
    {
        .instance = SPI2,
        .gpio = {
            .by_name = {
                .clk = { GPIOB, GPIO_PIN_10 },
                .miso = { GPIOC, GPIO_PIN_2 },
                .mosi = { GPIOC, GPIO_PIN_3 }
            }
        },
        .cs = {
            // Device 0
            { GPIOB, GPIO_PIN_9 },
            NO_MORE_GPIO_CFG
        },
        .mode = 0,
        .irq = SPI2_IRQn,
    },
    {
        .instance = SPI3,
        .gpio = {
            .by_name = {
                .clk = { GPIOC, GPIO_PIN_10 },
                .miso = { GPIOC, GPIO_PIN_11 },
                .mosi = { GPIOC, GPIO_PIN_12 }
            }
        },
        .cs = {
            { GPIOB, GPIO_PIN_15 },
            { GPIOB, GPIO_PIN_14 },
            { GPIOB, GPIO_PIN_13 },
            { GPIOB, GPIO_PIN_12 },
            { GPIOA, GPIO_PIN_4 },
            NO_MORE_GPIO_CFG
        },
        /* The datasheet of the MLX90393 magnetometer explicitly claims that
         * SPI mode 3 is implemented */
        .mode = 3,
        .irq = SPI3_IRQn,
    },
    NO_MORE_SPI_BUSES
};
#elif defined STM32F4
// STM32F4-Discovery
#define NUM_SPI_BUSES 0
const spi_bus_config_t spi_config[] = {
    NO_MORE_SPI_BUSES
};
#else
// No SPI buses supported on this hardware
#define NUM_SPI_BUSES 0
const spi_bus_config_t spi_config[] = {
    NO_MORE_SPI_BUSES
};
#endif
/* clang-format on */

static bool configure_spi_bus(spi_bus_state_t* state, const spi_bus_config_t* cfg);
static spi_bus_state_t* find_spi_bus_state(SPI_HandleTypeDef* hspi);
static bool is_spi_address_valid(teller::hal::spi::address_t address);

static SPI_HandleTypeDef* spi_handle_ptrs[7];
static spi_bus_state_t spi_state[NUM_SPI_BUSES];

static HAL_StatusTypeDef last_hal_status;

namespace teller::hal::spi {

const address_t NO_ADDRESS = { 0xFF, 0xFF };
const transfer_t NO_MORE_TRANSFERS = { 0, 0, 0 };

bool init()
{
    for (size_t i = 0; i < NUM_SPI_BUSES; i++) {
        if (!configure_spi_bus(&spi_state[i], &spi_config[i])) {
            return false;
        }
    }

    last_hal_status = HAL_OK;

    return true;
}

void destroy()
{
}

bool select(address_t address, bool value)
{
    const spi_bus_config_t* pCfg;
    const gpio_port_and_pins_t* csPinCfg;

    if (is_spi_address_valid(address)) {
        pCfg = &spi_config[address.bus];
        csPinCfg = &pCfg->cs[address.device];
        HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, value ? GPIO_PIN_RESET : GPIO_PIN_SET);
        return true;
    } else {
        return false;
    }
}

bool transfer(
    address_t address, std::uint8_t* buf, std::uint16_t size, std::uint8_t flags)
{
    return transfer(address, buf, buf, size, flags);
}

bool transfer(
    address_t address, std::uint8_t* txBuf, std::uint8_t* rxBuf, std::uint16_t size,
    std::uint8_t flags)
{
    const spi_bus_config_t* pCfg;
    const gpio_port_and_pins_t* csPinCfg;
    spi_bus_state_t* pState;
    bool success = false;
    std::uint32_t events;

    if (!is_spi_address_valid(address)) {
        return false;
    }

    if (txBuf == nullptr && rxBuf == nullptr) {
        return true;
    }

    pCfg = &spi_config[address.bus];
    csPinCfg = &pCfg->cs[address.device];

    pState = &spi_state[address.bus];

    if (osKernelGetState() == osKernelRunning) {
        // RTOS kernel is running so we can use event flags
        lock_guard lock(pState->in_use);

        if ((flags & NO_CHIP_SELECT) == 0) {
            HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, GPIO_PIN_RESET);
        }
        if (HAL_SPI_TransmitReceive_IT(&pState->handle, txBuf, rxBuf, size) == HAL_OK) {
            events = osEventFlagsWait(pState->event, EVT_DONE | EVT_ERROR, osFlagsWaitAny, SPI_TIMEOUT_MSEC);
            if (events & (osFlagsError | EVT_ERROR)) {
                HAL_SPI_Abort(&pState->handle);
            } else {
                success = events & EVT_DONE;
            }
        }
        if ((flags & NO_CHIP_SELECT) == 0) {
            HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, GPIO_PIN_SET);
        }
    } else {
        // Simplified implementation for the initialization where we cannot
        // use RTOS primitives yet
        if ((flags & NO_CHIP_SELECT) == 0) {
            HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, GPIO_PIN_RESET);
        }
        if (HAL_SPI_TransmitReceive(&pState->handle, txBuf, rxBuf, size, SPI_TIMEOUT_MSEC) == HAL_OK) {
            success = true;
        }
        if ((flags & NO_CHIP_SELECT) == 0) {
            HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, GPIO_PIN_SET);
        }
    }

    return success;
}

bool transfer(
    address_t address, const transfer_t* transfers, std::uint16_t count,
    std::uint8_t flags)
{
    const transfer_t* transfer;
    const spi_bus_config_t* pCfg;
    const gpio_port_and_pins_t* csPinCfg;
    spi_bus_state_t* pState;
    HAL_StatusTypeDef hal_status;
    std::uint32_t events;
    bool success = false;

    if (!is_spi_address_valid(address)) {
        return false;
    }

    pCfg = &spi_config[address.bus];
    csPinCfg = &pCfg->cs[address.device];

    pState = &spi_state[address.bus];

    if (count == 0) {
        count = std::numeric_limits<std::uint16_t>::max();
    }

    hal_status = HAL_OK;
    if (osKernelGetState() == osKernelRunning) {
        // RTOS kernel is running so we can use event flags
        lock_guard lock(pState->in_use);

        if ((flags & NO_CHIP_SELECT) == 0) {
            HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, GPIO_PIN_RESET);
        }

        transfer = transfers;
        while (count > 0) {
            count--;

            if (transfer->tx_buf == nullptr && transfer->rx_buf == nullptr) {
                success = true;
                break;
            }

            hal_status = HAL_SPI_TransmitReceive_IT(
                &pState->handle,
                transfer->tx_buf ? transfer->tx_buf : transfer->rx_buf,
                transfer->rx_buf ? transfer->rx_buf : transfer->tx_buf,
                transfer->size);

            if (hal_status != HAL_OK) {
                break;
            }

            events = osEventFlagsWait(pState->event, EVT_DONE | EVT_ERROR, osFlagsWaitAny, SPI_TIMEOUT_MSEC);
            if (events & (osFlagsError | EVT_ERROR)) {
                HAL_SPI_Abort(&pState->handle);
                break;
            }

            transfer++;
        }

        if ((flags & NO_CHIP_SELECT) == 0) {
            HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, GPIO_PIN_SET);
        }
    } else {
        // Simplified implementation for the initialization where we cannot
        // use RTOS primitives yet
        if ((flags & NO_CHIP_SELECT) == 0) {
            HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, GPIO_PIN_RESET);
        }

        transfer = transfers;
        while (count > 0) {
            count--;

            if (transfer->tx_buf != nullptr && transfer->rx_buf != nullptr) {
                hal_status = HAL_SPI_TransmitReceive(
                    &pState->handle, transfer->tx_buf, transfer->rx_buf,
                    transfer->size, SPI_TIMEOUT_MSEC);
            } else if (transfer->tx_buf != nullptr) {
                hal_status = HAL_SPI_Transmit(
                    &pState->handle, transfer->tx_buf, transfer->size, SPI_TIMEOUT_MSEC);
            } else if (transfer->rx_buf != nullptr) {
                hal_status = HAL_SPI_Receive(
                    &pState->handle, transfer->rx_buf, transfer->size, SPI_TIMEOUT_MSEC);
            } else {
                success = true;
                break;
            }

            if (hal_status != HAL_OK) {
                break;
            }

            transfer++;
        }

        if ((flags & NO_CHIP_SELECT) == 0) {
            HAL_GPIO_WritePin(csPinCfg->port, csPinCfg->pins, GPIO_PIN_SET);
        }
    }

    last_hal_status = hal_status;

    return success;
}

bool unselect(address_t address)
{
    return select(address, false);
}

int getLastErrorCode(void)
{
    return static_cast<int>(last_hal_status);
}

}

/* ************************************************************************** */

static bool configure_spi_bus(spi_bus_state_t* state, const spi_bus_config_t* cfg)
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

    /* SPI peripheral clock speed is set in board.cpp, here we divide it by
     * the baud rate prescaler.
     *
     * On the TELLER OBC board, the peripheral clock runs at 84 MHz for SPI1 and
     * 42 MHz for SPI2 and SPI3.
     *
     * For SPI1:
     *
     * Prescaler = 2: SPI bus will run at 42 MHz
     * Prescaler = 256: SPI bus will run at 328.125 kHz
     *
     * For SPI2:
     *
     * Prescaler = 2: SPI bus will run at 21 MHz
     * Prescaler = 256: SPI bus will run at 164.0625 kHz
     *
     * Note that a low clock rate is required to communicate with SD cards
     * in order to bring them into SPI mode. Online sources suggest a clock
     * rate between 100 and 400 kHz, so we cannot bump the initial clock rate
     * of the SPI2 bus higher.
     */

    /* Comment on converting SPI modes to CLKPolarity and CLKPhase.
     *
     * In SPI mode 0 and mode 1, the clock idles low. So the LSB of the mode
     * number determines the phase, and the other bit determines the polarity.
     *
     * CLKPolarity is SPI CPOL; CPOL=0 means that the clock idles low, so
     * that belongs to SPI_POLARITY_LOW.
     *
     * CLKPhase means whether we are sampling on the first or the second edge.
     * This is SPI CPHA; CPHA=0 means that we are sampling on the first edge,
     * so that belongs to SPI_PHASE_1EDGE.
     */
    pHandle->Init.Mode = SPI_MODE_MASTER;
    pHandle->Init.Direction = SPI_DIRECTION_2LINES;
    pHandle->Init.DataSize = SPI_DATASIZE_8BIT;
    pHandle->Init.CLKPolarity = (cfg->mode & 0x02) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    pHandle->Init.CLKPhase = (cfg->mode & 0x01) ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    pHandle->Init.NSS = SPI_NSS_SOFT;
    pHandle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    pHandle->Init.FirstBit = SPI_FIRSTBIT_MSB;
    pHandle->Init.TIMode = SPI_TIMODE_DISABLE;
    pHandle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    pHandle->Init.CRCPolynomial = 0x0;
#if defined(STM32H7)
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
#endif

    if (HAL_SPI_Init(pHandle) != HAL_OK) {
        goto cleanup;
    }

    /* HAL_SPI_MspInit sets state->initialized to true. If this is still false
     * at this point, it means that the initialization was not successful */
    if (!state->initialized) {
        goto cleanup;
    }

    state->event = osEventFlagsNew(nullptr);
    if (!state->event) {
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
static spi_bus_state_t* find_spi_bus_state(SPI_HandleTypeDef* hspi)
{
    for (int index = 0; index < NUM_SPI_BUSES; index++) {
        if (hspi == &spi_state[index].handle) {
            return &spi_state[index];
        }
    }

    return nullptr;
}

/* Returns whether an SPI address is valid */
static bool is_spi_address_valid(teller::hal::spi::address_t address)
{
    if (address.bus >= NUM_SPI_BUSES || !spi_state[address.bus].initialized) {
        return false;
    }

    if (address.device >= NUM_CS_PINS_PER_BUS || !spi_config[address.bus].cs[address.device].port) {
        return false;
    }

    return true;
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

    spi_bus_state_t* state = find_spi_bus_state(hspi);
    const spi_bus_config_t* cfg = state ? state->cfg : nullptr;
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
    } else if (hspi->Instance == SPI3) {
        gpioFunc = GPIO_AF6_SPI3;

#ifdef STM32H7
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

        /* Initializes the peripherals clock */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI3;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            return;
        }
#endif

        /* Peripheral clock enable */
        __HAL_RCC_SPI3_CLK_ENABLE();
    }

    /* GPIO configuration */
    for (int i = 0; i < NUM_GPIO_PINS_PER_BUS; i++) {
        if (cfg->gpio.by_index[i].port && cfg->gpio.by_index[i].pins) {
            enableGPIOClocksForPort(cfg->gpio.by_index[i].port);
            GPIO_InitStruct.Pin = cfg->gpio.by_index[i].pins;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            GPIO_InitStruct.Alternate = gpioFunc;
            HAL_GPIO_Init(cfg->gpio.by_index[i].port, &GPIO_InitStruct);
        }
    }

    /* Chip select pin configuration */
    for (int i = 0; i < NUM_CS_PINS_PER_BUS; i++) {
        if (cfg->cs[i].port && cfg->cs[i].pins) {
            enableGPIOClocksForPort(cfg->cs[i].port);
            GPIO_InitStruct.Pin = cfg->cs[i].pins;
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            HAL_GPIO_Init(cfg->cs[i].port, &GPIO_InitStruct);
            HAL_GPIO_WritePin(cfg->cs[i].port, cfg->cs[i].pins, GPIO_PIN_SET);
        }
    }

    /* IRQ configuration */
    if (cfg->irq) {
        /* Priority 5 is the highest (i.e. smallest numeric value) that is
         * allowed without interfering with FreeRTOS */
        HAL_NVIC_SetPriority(cfg->irq, 5, 0);
        HAL_NVIC_EnableIRQ(cfg->irq);
    }

    state->initialized = true;

    if (hspi->Instance == SPI1) {
        spi_handle_ptrs[1] = hspi;
    } else if (hspi->Instance == SPI2) {
        spi_handle_ptrs[2] = hspi;
    } else if (hspi->Instance == SPI3) {
        spi_handle_ptrs[3] = hspi;
    }
}

/* Weakly linked function that is called by the STM32 HAL when an SPI bus is
 * deinitialized
 */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
    spi_bus_state_t* state = find_spi_bus_state(hspi);
    const spi_bus_config_t* cfg = state ? state->cfg : nullptr;
    if (!cfg) {
        return;
    }

    if (hspi->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_DISABLE();
    } else if (hspi->Instance == SPI2) {
        __HAL_RCC_SPI2_CLK_DISABLE();
    } else if (hspi->Instance == SPI3) {
        __HAL_RCC_SPI3_CLK_DISABLE();
    }

    for (int i = NUM_GPIO_PINS_PER_BUS - 1; i >= 0; i--) {
        if (cfg->gpio.by_index[i].port && cfg->gpio.by_index[i].pins) {
            HAL_GPIO_DeInit(cfg->gpio.by_index[i].port, cfg->gpio.by_index[i].pins);
        }
    }

    for (int i = NUM_CS_PINS_PER_BUS - 1; i >= 0; i--) {
        if (cfg->cs[i].port && cfg->cs[i].pins) {
            HAL_GPIO_DeInit(cfg->cs[i].port, cfg->cs[i].pins);
        }
    }

    if (cfg->irq) {
        HAL_NVIC_DisableIRQ(cfg->irq);
    }

    if (state) {
        osEventFlagsDelete(state->event);
        state->initialized = false;
    }

    if (hspi->Instance == SPI1) {
        spi_handle_ptrs[1] = nullptr;
    } else if (hspi->Instance == SPI2) {
        spi_handle_ptrs[2] = nullptr;
    } else if (hspi->Instance == SPI3) {
        spi_handle_ptrs[3] = nullptr;
    }
}

void SPI1_IRQHandler(void)
{
    SPI_HandleTypeDef* ptr = spi_handle_ptrs[1];
    if (ptr) {
        HAL_SPI_IRQHandler(ptr);
    }
}

void SPI2_IRQHandler(void)
{
    SPI_HandleTypeDef* ptr = spi_handle_ptrs[2];
    if (ptr) {
        HAL_SPI_IRQHandler(ptr);
    }
}

void SPI3_IRQHandler(void)
{
    SPI_HandleTypeDef* ptr = spi_handle_ptrs[3];
    if (ptr) {
        HAL_SPI_IRQHandler(ptr);
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* spi)
{
    spi_bus_state_t* state = find_spi_bus_state(spi);
    if (state) {
        osEventFlagsSet(state->event, EVT_DONE);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* spi)
{
    spi_bus_state_t* state = find_spi_bus_state(spi);
    if (state) {
        osEventFlagsSet(state->event, EVT_ERROR);
    }
}
}
