#include <cassert>
#include <cstring>
#include <stdexcept>

#include "stm32_hal.h"
#include <cmsis_os2.h>

#include "config.h"
#include "hal/uart.h"
#include "utils.h"

using namespace std;
using namespace teller::hal::uart;
using namespace teller::hal::utils;

typedef struct {
    USART_TypeDef* instance;
    GPIO_TypeDef* gpio_port;
    uint32_t gpio_pins;
    IRQn_Type irq;

    uint32_t baud_rate;
    bool hw_flow_control;
} uart_phy_config_t;

typedef struct {
    UART_HandleTypeDef handle;
    const uart_phy_config_t* cfg;
    osEventFlagsId_t event;
    bool initialized;
} uart_phy_state_t;

static const uint32_t EVT_READ = 0x00000001U;
static const uint32_t EVT_WRITTEN = 0x00000002U;

/* NOTE: Each UART must be mapped to a different UART_HandleTypeDef. This is
 * not checked explicitly. */

#define NO_MORE_UARTS \
    {                 \
        0             \
    }

#if defined TELLER_BOARD_NUCLEO144
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#define NUM_PHY_UARTS 1
const uart_phy_config_t uart_phy_config[] = {
    { USART3, GPIOD, GPIO_PIN_8 | GPIO_PIN_9, USART3_IRQn },
    NO_MORE_UARTS
};
const int8_t uart_map[NUM_UARTS] = { 0, 0 };
#elif defined STM32F4
// STM32F4-Discovery
#define NUM_PHY_UARTS 0
const uart_phy_config_t uart_phy_config[] = {
    NO_MORE_UARTS
};
const int8_t uart_map[NUM_UARTS] = { -1, -1 };
#else
// No UART supported on this hardware
#define NUM_PHY_UARTS 0
const uart_phy_config_t uart_phy_config[NUM_UARTS] = {
    NO_MORE_UARTS
};
const int8_t uart_map[NUM_UARTS] = { -1, -1 };
#endif

static bool configure_uart_phy(uart_phy_state_t* state, const uart_phy_config_t* cfg);
static uart_phy_state_t* find_phy_for_uart(int8_t index);

static UART_HandleTypeDef* uart_handle_ptrs[10];
static uart_phy_state_t uart_phy_state[NUM_PHY_UARTS];

bool teller::hal::uart::init()
{
    for (size_t i = 0; i < NUM_PHY_UARTS; i++) {
        if (!configure_uart_phy(&uart_phy_state[i], &uart_phy_config[i])) {
            return false;
        }
    }

    return true;
}

void teller::hal::uart::destroy()
{
    /* Not needed; we never call the destructor in STM32 */
}

bool teller::hal::uart::read(uart_t index, uint8_t* data, uint16_t size, uint16_t* bytes_read)
{
    if (size == 0) {
        if (bytes_read) {
            *bytes_read = 0;
        }
        return true;
    }

    uart_phy_state_t* pState = find_phy_for_uart(index);
    if (pState) {
        UART_HandleTypeDef* pHandle = &pState->handle;

        if (HAL_UART_Receive_IT(pHandle, data, size) != HAL_OK) {
            return false;
        }

        osEventFlagsWait(pState->event, EVT_READ, osFlagsWaitAny, osWaitForever);

        if (bytes_read) {
            *bytes_read = size;
        }
    } else {
        if (bytes_read) {
            *bytes_read = 0;
        }
    }

    return true;
}

bool teller::hal::uart::write(uart_t index, uint8_t* data, uint16_t size)
{
    uart_phy_state_t* pState = find_phy_for_uart(index);
    if (pState) {
        UART_HandleTypeDef* pHandle = &pState->handle;

        if (HAL_UART_Transmit_IT(pHandle, data, size) != HAL_OK) {
            return false;
        }

        osEventFlagsWait(pState->event, EVT_WRITTEN, osFlagsWaitAny, osWaitForever);
    }

    return true;
}

bool teller::hal::uart::write(uart_t index, const char* data)
{
    return write(index, reinterpret_cast<uint8_t*>(const_cast<char*>(data)), strlen(data));
}

/* ************************************************************************** */

static bool configure_uart_phy(uart_phy_state_t* state, const uart_phy_config_t* cfg)
{
    bool success = false;

    if (state->initialized || (state->cfg && state->cfg != cfg)) {
        return false;
    }

    state->cfg = cfg;

    if (cfg->instance == nullptr) {
        return true;
    }

    UART_HandleTypeDef* pHandle = &state->handle;

    pHandle->Instance = cfg->instance;
    pHandle->Init.BaudRate = cfg->baud_rate > 0 ? cfg->baud_rate : 38400;
    pHandle->Init.WordLength = UART_WORDLENGTH_8B;
    pHandle->Init.StopBits = UART_STOPBITS_1;
    pHandle->Init.Parity = UART_PARITY_NONE;
    pHandle->Init.Mode = UART_MODE_TX_RX;
    pHandle->Init.HwFlowCtl = cfg->hw_flow_control ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
    pHandle->Init.OverSampling = UART_OVERSAMPLING_16;

#ifdef TELLER_TARGET_MCU_STM32H7
    pHandle->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    pHandle->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    pHandle->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif

    if (HAL_UART_Init(pHandle) != HAL_OK) {
        goto cleanup;
    }

    /* HAL_UART_MspInit sets state->initialized to true. If this is still false
     * at this point, it means that the initialization was not successful */
    if (!state->initialized) {
        goto cleanup;
    }

#ifdef STM32H7
    if (HAL_UARTEx_SetTxFifoThreshold(pHandle, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        goto cleanup;
    }

    if (HAL_UARTEx_SetRxFifoThreshold(pHandle, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        goto cleanup;
    }

    if (HAL_UARTEx_DisableFifoMode(pHandle) != HAL_OK) {
        goto cleanup;
    }
#endif

    state->event = osEventFlagsNew(nullptr);
    if (!state->event) {
        goto cleanup;
    }

    success = true;

cleanup:
    if (state->initialized && !success) {
        HAL_UART_DeInit(pHandle);
        state->initialized = false;
    }

    return success;
}

static uart_phy_state_t* find_phy_for_uart(int8_t index)
{
    assert(index >= 0 && index < NUM_UARTS);

    int8_t uart_phy_index = uart_map[index];
    return uart_phy_index >= 0 ? &uart_phy_state[uart_phy_index] : nullptr;
}

/* ************************************************************************** */

/* Finds the UART configuration corresponding to the given physical UART */
static uart_phy_state_t* find_uart_phy_state(UART_HandleTypeDef* huart)
{
    for (int index = 0; index < NUM_PHY_UARTS; index++) {
        if (huart == &uart_phy_state[index].handle) {
            return &uart_phy_state[index];
        }
    }

    return nullptr;
}

/* Weakly linked function that is called by the STM32 HAL when a physical UART is
 * initialized
 */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    uart_phy_state_t* state = find_uart_phy_state(huart);
    const uart_phy_config_t* cfg = state ? state->cfg : nullptr;
    if (!cfg) {
        return;
    }

    if (huart->Instance == USART3) {
#ifdef STM32H7
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

        /* Initializes the peripherals clock */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
        PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            return;
        }
#endif

        /* Peripheral clock enable */
        __HAL_RCC_USART3_CLK_ENABLE();
    }

    /* GPIO configuration */
    if (cfg->gpio_port && cfg->gpio_pins) {
        enableGPIOClocksForPort(cfg->gpio_port);
        GPIO_InitStruct.Pin = cfg->gpio_pins;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(cfg->gpio_port, &GPIO_InitStruct);
    }

    /* IRQ configuration */
    if (cfg->irq) {
        /* Priority 5 is the highest (i.e. smallest numeric value) that is
         * allowed without interfering with FreeRTOS */
        HAL_NVIC_SetPriority(cfg->irq, 5, 0);
        HAL_NVIC_EnableIRQ(cfg->irq);
    }

    state->initialized = true;

    if (huart->Instance == USART3) {
        uart_handle_ptrs[3] = huart;
    }
}

/* Weakly linked function that is called by the STM32 HAL when an UART is
 * deinitialized
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
    uart_phy_state_t* state = find_uart_phy_state(huart);
    const uart_phy_config_t* cfg = state ? state->cfg : nullptr;
    if (!cfg) {
        return;
    }

    if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_DISABLE();
    }

    if (cfg->gpio_port && cfg->gpio_pins) {
        HAL_GPIO_DeInit(cfg->gpio_port, cfg->gpio_pins);
    }

    if (cfg->irq) {
        HAL_NVIC_DisableIRQ(cfg->irq);
    }

    if (state) {
        osEventFlagsDelete(state->event);
        state->initialized = false;
    }

    if (huart->Instance == USART3) {
        uart_handle_ptrs[3] = nullptr;
    }
}

/* ************************************************************************** */

extern "C" {

void USART3_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[3];
    if (ptr) {
        HAL_UART_IRQHandler(ptr);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* uart)
{
    uart_phy_state_t* state = find_uart_phy_state(uart);
    if (state) {
        osEventFlagsSet(state->event, EVT_WRITTEN);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* uart)
{
    uart_phy_state_t* state = find_uart_phy_state(uart);
    if (state) {
        osEventFlagsSet(state->event, EVT_READ);
    }
}

int __io_putchar(int ch)
{
    uint8_t to_write = ch;
    teller::hal::uart::write(DEBUG, &to_write, 1);
    return ch;
}
}
