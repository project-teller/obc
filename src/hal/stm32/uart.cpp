#include <cassert>
#include <cstring>
#include <stdexcept>

#include "stm32_hal.h"
#include <cmsis_os2.h>

#include "config.h"
#include "hal/dma.h"
#include "hal/stm32/utils.h"
#include "hal/system.h"
#include "hal/uart.h"

using namespace std;
using namespace teller::hal::uart;
using namespace teller::hal::utils;

namespace teller::hal::uart {

const uint32_t WAIT_FOREVER = osWaitForever;

}

static const uint32_t EVT_READ = 0x00000001U;
static const uint32_t EVT_WRITTEN = 0x00000002U;
static const uint32_t EVT_ERROR = 0x00000004U;

#define NUM_GPIO_PINS_PER_UART 2

typedef struct {
    /** The physical STM32 HAL UART instance being configured by this entry */
    USART_TypeDef* instance;

    /** GPIO pins that the UART will use */
    union {
        gpio_port_and_pins_t by_index[NUM_GPIO_PINS_PER_UART];
        struct {
            gpio_port_and_pins_t tx;
            gpio_port_and_pins_t rx;
        } by_name;
    } gpio;

    /**
     * IRQ that the UART will use to notify us about incoming data. Zero if
     * the UART is using DMA.
     */
    IRQn_Type irq;

    /**
     * The DMA channel to use for TX operations. Null if the UART is not using
     * DMA for TX.
     */
    DMA_Stream_TypeDef* dma_tx;

    /**
     * The DMA channel to use for RX operations. Null if the UART is not using
     * DMA for TX.
     */
    DMA_Stream_TypeDef* dma_rx;

    /** Baud rate to configure the UART for */
    uint32_t baud_rate;

    /** Whether hardware flow control should be enabled on the UART */
    bool hw_flow_control;
} uart_phy_config_t;

typedef struct {
    /** Handle to the configured STM32 UART instance */
    UART_HandleTypeDef handle;

    /** Pointer to the physical UART configuration */
    const uart_phy_config_t* cfg;

    /** DMA stream of the UART when it is using DMA for TX */
    DMA_HandleTypeDef dma_tx_handle;

    /** DMA stream of the UART when it is using DMA for RX */
    DMA_HandleTypeDef dma_rx_handle;

    /**
     * Event flags to trigger when the UART is ready to read or write or if an
     * error happened.
     */
    osEventFlagsId_t event;

    /** Whether this state object is initialized or not */
    bool initialized;
} uart_phy_state_t;

/* NOTE: Each UART must be mapped to a different UART_HandleTypeDef. This is
 * not checked explicitly. */

#define NO_MORE_UARTS \
    {                 \
        0             \
    }

/* clang-format off */
#if defined TELLER_BOARD_NUCLEO144
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#define NUM_PHY_UARTS 2
const uart_phy_config_t uart_phy_config[] = {
    /* UART towards the RXSM */
    {
        .instance = USART2,
        .gpio = {
            .by_name = {
                .tx = { GPIOD, GPIO_PIN_5 },
                .rx = { GPIOA, GPIO_PIN_3 }
            },
        },
        .irq = USART2_IRQn,
        .dma_tx = nullptr,
        .dma_rx = nullptr,
        .baud_rate = 38400
    },

    /* Debug UART */
    {
        .instance = USART3,
        .gpio = {
            .by_name = {
                .tx = { GPIOD, GPIO_PIN_8 },
                .rx = { GPIOD, GPIO_PIN_9 }
            },
        },
        .irq = USART3_IRQn,
        .dma_tx = nullptr,
        .dma_rx = nullptr,
        .baud_rate = 115200  /* 115200 is the max that seems to work; 230400 triggers the watchdog sometimes */
    },
    NO_MORE_UARTS
};
const int8_t uart_map[NUM_UARTS] = {
    0,   /* TELEMETRY --> USART2 */
    -1,  /* GMM --> not connected */
    -1,  /* SCM --> not connected */
    1,   /* DEBUG --> USART3 */
    -1   /* SINK --> not connected */
};
#elif defined STM32F4
// STM32F4-Discovery
#define NUM_PHY_UARTS 0
const uart_phy_config_t uart_phy_config[] = {
    NO_MORE_UARTS
};
const int8_t uart_map[NUM_UARTS] = { -1, -1, -1, -1, -1 };
#else
// No UART supported on this hardware
#define NUM_PHY_UARTS 0
const uart_phy_config_t uart_phy_config[NUM_UARTS] = {
    NO_MORE_UARTS
};
const int8_t uart_map[NUM_UARTS] = { -1, -1, -1, -1, -1 };
#endif
/* clang-format on */

static bool configure_uart_phy(uart_phy_state_t* state, const uart_phy_config_t* cfg);
static uart_phy_state_t* find_phy_for_uart(int8_t index);

static bool isUARTAlwaysConnected(uart_t index);

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

bool teller::hal::uart::isConnected(uart_t index)
{
    if (uart_map[index] < 0) {
        return false;
    }

    if (isUARTAlwaysConnected(index)) {
        return true;
    }

    /* TODO: detect when the debug UART is connected or disconnected */
    return index != DEBUG;
}

bool teller::hal::uart::readInto(uart_t index, uint8_t* data, uint16_t size, uint16_t* bytes_read)
{
    uint32_t flags = 0;
    uart_phy_state_t* pState;
    UART_HandleTypeDef* pHandle;

    if (size == 0) {
        teller::hal::system::yield();
        goto exit;
    }

    pState = find_phy_for_uart(index);
    pHandle = pState ? &pState->handle : nullptr;

    if (pHandle) {
        if (HAL_UART_Receive_IT(pHandle, data, size) == HAL_OK) {
            flags = osEventFlagsWait(pState->event, EVT_READ | EVT_ERROR, osFlagsWaitAny, osWaitForever);
            if (flags & (osFlagsError | EVT_ERROR)) {
                HAL_UART_AbortReceive(pHandle);
            }
        } else {
            teller::hal::system::yield();
        }
    } else {
        teller::hal::system::yield();
    }

exit:
    if (flags & EVT_READ) {
        if (bytes_read) {
            *bytes_read = size;
        }
        return true;
    } else {
        return false;
    }
}

bool teller::hal::uart::read1(uart_t index, uint8_t* data, uint32_t timeout)
{
    uart_phy_state_t* pState = find_phy_for_uart(index);
    UART_HandleTypeDef* pHandle = pState ? &pState->handle : nullptr;
    HAL_StatusTypeDef result;
    uint32_t flags = 0;
    bool shouldAbort;

    if (!pHandle) {
        teller::hal::system::yield();
        goto exit;
    }

    result = HAL_UART_Receive_IT(pHandle, data, 1);
    if (result == HAL_BUSY) {
        if (timeout == 0) {
            /* We were just polling, so yield and return false */
            teller::hal::system::yield();
            goto exit;
        } else if (timeout == WAIT_FOREVER) {
            /* Retry until successful */
            while (result == HAL_BUSY) {
                teller::hal::system::delayMsec(1);
                result = HAL_UART_Receive_IT(pHandle, data, 1);
            }
        } else {
            /* Try again until the timeout expires */
            uint32_t now = teller::hal::system::getTimeSinceBootMsec();
            uint32_t deadline = now + timeout;
            while (result == HAL_BUSY && now < deadline) {
                teller::hal::system::delayMsec(1);
                result = HAL_UART_Receive_IT(pHandle, data, 1);
                now = teller::hal::system::getTimeSinceBootMsec();
            }
        }
    }

    if (result == HAL_OK) {
        flags = osEventFlagsWait(pState->event, EVT_READ | EVT_ERROR, osFlagsWaitAny, timeout);
        shouldAbort = flags & (osFlagsError | EVT_ERROR);
        if (shouldAbort) {
            HAL_UART_AbortReceive(pHandle);
        }
    } else {
        HAL_UART_AbortReceive(pHandle);
        teller::hal::system::yield();
    }

exit:
    return flags & EVT_READ;
}

void teller::hal::uart::waitUntilConnected(uart_t index)
{
    if (isUARTAlwaysConnected(index)) {
        return;
    } else {
        while (!isConnected(index)) {
            teller::hal::system::delayMsec(100);
        }
    }
}

void teller::hal::uart::waitUntilDisconnected(uart_t index)
{
    if (isUARTAlwaysConnected(index)) {
        teller::hal::system::sleepForever();
    } else {
        while (isConnected(index)) {
            teller::hal::system::delayMsec(100);
        }
    }
}

bool teller::hal::uart::write(uart_t index, uint8_t* data, uint16_t size)
{
    uint32_t flags;
    uart_phy_state_t* pState = find_phy_for_uart(index);

    if (pState && pState->cfg) {
        UART_HandleTypeDef* pHandle = &pState->handle;

        if (pState->cfg->dma_tx) {
            /* Transfer with DMA */
            /* TODO(ntamas): this does not work yet; DMA needs a buffer that is
             * guaranteed to be in DMA-accessible memory */
            if (HAL_UART_Transmit_DMA(pHandle, data, size) != HAL_OK) {
                return false;
            }
        } else {
            /* Transfer with interrupts */
            if (HAL_UART_Transmit_IT(pHandle, data, size) != HAL_OK) {
                return false;
            }
        }

        flags = osEventFlagsWait(pState->event, EVT_WRITTEN | EVT_ERROR, osFlagsWaitAny, osWaitForever);

        if (flags & (osFlagsError | EVT_ERROR)) {
            HAL_UART_AbortTransmit(pHandle);
            return false;
        }
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

static bool isUARTAlwaysConnected(uart_t index)
{
    return true; // index != DEBUG;
}

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

static void assignHandle(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART1) {
        uart_handle_ptrs[1] = huart;
    } else if (huart->Instance == USART2) {
        uart_handle_ptrs[2] = huart;
    } else if (huart->Instance == USART3) {
        uart_handle_ptrs[3] = huart;
    }
}

static void detachHandle(UART_HandleTypeDef* huart)
{
    int i = 0, n = sizeof(uart_handle_ptrs) / sizeof(uart_handle_ptrs[0]);
    for (i = 0; i < n; i++) {
        if (huart->Instance == uart_handle_ptrs[i]->Instance) {
            uart_handle_ptrs[i] = nullptr;
        }
    }
}

/* ************************************************************************** */

namespace teller::hal::dma {

void assignHandle(DMA_HandleTypeDef* handle);
void detachHandle(DMA_HandleTypeDef* handle);

}

extern "C" {

/* Weakly linked function that is called by the STM32 HAL when a physical UART is
 * initialized
 */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    uint8_t gpioFunc = 0;

    uart_phy_state_t* state = find_uart_phy_state(huart);
    const uart_phy_config_t* cfg = state ? state->cfg : nullptr;
    if (!cfg) {
        return;
    }

    if (huart->Instance == USART1) {
        gpioFunc = GPIO_AF7_USART1;

#ifdef STM32H7
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

        /* Initializes the peripherals clock */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            return;
        }
#endif

        /* Peripheral clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();
    } else if (huart->Instance == USART2) {
        gpioFunc = GPIO_AF7_USART2;

#ifdef STM32H7
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

        /* Initializes the peripherals clock */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART2;
        PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            return;
        }
#endif

        /* Peripheral clock enable */
        __HAL_RCC_USART2_CLK_ENABLE();
    } else if (huart->Instance == USART3) {
        gpioFunc = GPIO_AF7_USART3;

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
    for (int i = 0; i < NUM_GPIO_PINS_PER_UART; i++) {
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

    /* IRQ configuration */
    if (cfg->irq) {
        /* Priority 5 is the highest (i.e. smallest numeric value) that is
         * allowed without interfering with FreeRTOS */
        HAL_NVIC_SetPriority(cfg->irq, 5, 0);
        HAL_NVIC_EnableIRQ(cfg->irq);
    }

    /* DMA configuration */
    if (cfg->dma_tx) {
        state->dma_tx_handle.Instance = cfg->dma_tx;

        if (huart->Instance == USART1) {
            state->dma_tx_handle.Init.Request = DMA_REQUEST_USART1_TX;
        } else if (huart->Instance == USART2) {
            state->dma_tx_handle.Init.Request = DMA_REQUEST_USART2_TX;
        } else if (huart->Instance == USART3) {
            state->dma_tx_handle.Init.Request = DMA_REQUEST_USART3_TX;
        } else {
            return;
        }

        state->dma_tx_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        state->dma_tx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
        state->dma_tx_handle.Init.MemInc = DMA_MINC_ENABLE;
        state->dma_tx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        state->dma_tx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        state->dma_tx_handle.Init.Mode = DMA_NORMAL;
        state->dma_tx_handle.Init.Priority = DMA_PRIORITY_LOW;
        state->dma_tx_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&state->dma_tx_handle) != HAL_OK) {
            state->dma_tx_handle.Instance = nullptr; /* to prevent HAL_DMA_Deinit */
            return;
        }

        __HAL_LINKDMA(huart, hdmatx, state->dma_tx_handle);
    } else {
        state->dma_tx_handle.Instance = nullptr;
    }

    state->initialized = true;

    assignHandle(huart);

    if (state->dma_tx_handle.Instance) {
        teller::hal::dma::assignHandle(&state->dma_tx_handle);
    };
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

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_DISABLE();
    } else if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_DISABLE();
    } else if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_DISABLE();
    }

    for (int i = NUM_GPIO_PINS_PER_UART - 1; i >= 0; i--) {
        if (cfg->gpio.by_index[i].port && cfg->gpio.by_index[i].pins) {
            HAL_GPIO_DeInit(cfg->gpio.by_index[i].port, cfg->gpio.by_index[i].pins);
        }
    }

    if (cfg->irq) {
        HAL_NVIC_DisableIRQ(cfg->irq);
    }

    if (cfg->dma_tx && state->dma_tx_handle.Instance) {
        HAL_DMA_DeInit(&state->dma_tx_handle);
    }

    if (state) {
        osEventFlagsDelete(state->event);
        state->initialized = false;
    }

    detachHandle(huart);

    if (state->dma_tx_handle.Instance) {
        teller::hal::dma::detachHandle(&state->dma_tx_handle);
    };
}
}

/* ************************************************************************** */

/* IRQ handlers */

extern "C" {

void USART1_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[1];
    if (ptr) {
        HAL_UART_IRQHandler(ptr);
    }
}

void USART2_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[2];
    if (ptr) {
        HAL_UART_IRQHandler(ptr);
    }
}

void USART3_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[3];
    if (ptr) {
        HAL_UART_IRQHandler(ptr);
    }
}
}

/* ************************************************************************** */

/* HAL UART callbacks */

extern "C" {

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

void HAL_UART_ErrorCallback(UART_HandleTypeDef* uart)
{
    uart_phy_state_t* state = find_uart_phy_state(uart);
    if (state) {
        osEventFlagsSet(state->event, EVT_ERROR);
    }
}
}

/* ************************************************************************** */

/* printf integration */

extern "C" {

int __io_putchar(int ch)
{
    uint8_t to_write = ch;
    teller::hal::uart::write(DEBUG, &to_write, 1);
    return ch;
}
}
