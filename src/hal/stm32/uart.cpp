#include <cassert>
#include <cstring>
#include <stdexcept>

#include "lwrb/lwrb.h"
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
     * DMA for RX.
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

    /** Ring buffer that contains the data being sent on this UART */
    lwrb_t tx_buffer;

    /** Ring buffer that contains the data being received from this UART */
    lwrb_t rx_buffer;

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
    /* USART2 towards the RXSM */
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

    /* USART3 towards the debug port */
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
        .baud_rate = 230400  /* 230400 is the max that seems to work */
    },
    NO_MORE_UARTS
};
const int8_t uart_map[NUM_UARTS] = {
    0,   /* RXSM --> USART2 */
    -1,  /* GMM --> not connected */
    -1,  /* SCM --> not connected */
    1,   /* DEBUG --> USART3 */
    -1   /* SINK --> not connected */
};
#elif defined TELLER_BOARD_STM32F4
// STM32F415RG TELLER OBC
#define NUM_PHY_UARTS 3
const uart_phy_config_t uart_phy_config[] = {
    /* USART1 towards the SCM */
    {
        .instance = USART1,
        .gpio = {
            .by_name = {
                .tx = { GPIOA, GPIO_PIN_9 },
                .rx = { GPIOA, GPIO_PIN_8 }
            },
        },
        .irq = USART1_IRQn,
        .dma_tx = nullptr,
        .dma_rx = nullptr,
        .baud_rate = 38400
    },

    /* UART4 towards the RXSM */
    {
        .instance = UART4,
        .gpio = {
            .by_name = {
                .tx = { GPIOA, GPIO_PIN_0 },
                .rx = { GPIOA, GPIO_PIN_1 }
            },
        },
        .irq = UART4_IRQn,
        .dma_tx = nullptr,
        .dma_rx = nullptr,
        .baud_rate = 38400
    },

    /* USART6 towards the GMM */
    {
        .instance = USART6,
        .gpio = {
            .by_name = {
                .tx = { GPIOC, GPIO_PIN_6 },
                .rx = { GPIOC, GPIO_PIN_7 }
            },
        },
        .irq = USART6_IRQn,
        .dma_tx = nullptr,
        .dma_rx = nullptr,
        .baud_rate = 38400
    },

    NO_MORE_UARTS
};
const int8_t uart_map[NUM_UARTS] = {
    1,   /* RXSM --> UART4 */
    2,  /* GMM --> UART6 */
    0,  /* SCM --> USART1 */
    -1,   /* DEBUG --> not connected, USB-OTG */
    -1   /* SINK --> not connected */
};
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
static void uart_irq_handler(UART_HandleTypeDef* ptr);

static bool isUARTAlwaysConnected(uart_t index);

static UART_HandleTypeDef* uart_handle_ptrs[10];
static uart_phy_state_t uart_phy_state[NUM_PHY_UARTS];

#define DMA_BUFFER_SIZE 256

static DMA_BUFFER uint8_t uart_rx_buffers[NUM_PHY_UARTS][DMA_BUFFER_SIZE];
static DMA_BUFFER uint8_t uart_tx_buffers[NUM_PHY_UARTS][DMA_BUFFER_SIZE];

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
    uart_phy_state_t* state;
    lwrb_sz_t read = 0;

    if (size == 0) {
        teller::hal::system::yield();
        goto exit;
    }

    state = find_phy_for_uart(index);
    if (!state) {
        teller::hal::system::yield();
        goto exit;
    }

    while (read == 0) {
        /* Wait until there is something in the buffer */
        flags = osEventFlagsWait(state->event, EVT_READ, osFlagsWaitAny, osWaitForever);
        if (flags & EVT_READ) {
            /* Try reading from the RX buffer */
            read = lwrb_read(&state->rx_buffer, data, size);
        }
    }

exit:
    if (bytes_read) {
        *bytes_read = read;
    }
    return true;
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
    uart_phy_state_t* state;
    lwrb_sz_t written;

    if (size == 0) {
        teller::hal::system::yield();
        goto exit;
    }

    state = find_phy_for_uart(index);
    if (!state) {
        teller::hal::system::yield();
        goto exit;
    }

    /* We do not use the STM32 HAL for UART handling; registers are
     * programmed directly to address the shortcomings of the STM32 HAL */

    /* Push the data to write into the buffer */
    while (size > 0) {
        written = lwrb_write(&state->tx_buffer, data, size);
        size -= written;
        data += written;

        /* Enable TXEIE to get a notification when the UART is ready to send,
         * and then wait for the event that indicates that the ring buffer is
         * empty again */
        SET_BIT(state->handle.Instance->CR1, USART_CR1_TXEIE);

        /* TODO: figure out why we need a timeout of 100 ms here, why we cannot
         * use osWaitForever. Doing so would stall the outbound queue in an
         * echo test */
        osEventFlagsWait(state->event, EVT_WRITTEN, osFlagsWaitAny, 100);
    }

exit:
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

    /* Enable RXNEIE to notify us when a new byte can be received */
    SET_BIT(pHandle->Instance->CR1, USART_CR1_RXNEIE);

    /* Make sure that parity errors (PE) and transmission complete events (TC)
     * do not trigger interrupts */
    CLEAR_BIT(pHandle->Instance->CR1, USART_CR1_PEIE | USART_CR1_TCIE);

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
    } else if (huart->Instance == UART4) {
        uart_handle_ptrs[4] = huart;
    } else if (huart->Instance == UART5) {
        uart_handle_ptrs[5] = huart;
    } else if (huart->Instance == USART6) {
        uart_handle_ptrs[6] = huart;
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

#ifdef STM32H7
#define INIT_PERIPH_CLOCK(clockSelection)                          \
    {                                                              \
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };      \
                                                                   \
        /* Initializes the peripherals clock */                    \
        PeriphClkInitStruct.PeriphClockSelection = clockSelection; \
        PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        return;
    }
#else
#define INIT_PERIPH_CLOCK(clockSelection) ;
#endif

    if (huart->Instance == USART1) {
        gpioFunc = GPIO_AF7_USART1;
        INIT_PERIPH_CLOCK(RCC_USART234578CLKSOURCE_D2PCLK1);
        __HAL_RCC_USART1_CLK_ENABLE();
    } else if (huart->Instance == USART2) {
        gpioFunc = GPIO_AF7_USART2;
        INIT_PERIPH_CLOCK(RCC_USART234578CLKSOURCE_D2PCLK1);
        __HAL_RCC_USART2_CLK_ENABLE();
    } else if (huart->Instance == USART3) {
        gpioFunc = GPIO_AF7_USART3;
        INIT_PERIPH_CLOCK(RCC_USART234578CLKSOURCE_D2PCLK1);
        __HAL_RCC_USART3_CLK_ENABLE();
    } else if (huart->Instance == UART4) {
        gpioFunc = GPIO_AF8_UART4;
        INIT_PERIPH_CLOCK(RCC_USART234578CLKSOURCE_D2PCLK1);
        __HAL_RCC_UART4_CLK_ENABLE();
    } else if (huart->Instance == UART5) {
        gpioFunc = GPIO_AF8_UART5;
        INIT_PERIPH_CLOCK(RCC_USART234578CLKSOURCE_D2PCLK1);
        __HAL_RCC_UART5_CLK_ENABLE();
    } else if (huart->Instance == USART6) {
        gpioFunc = GPIO_AF8_USART6;
        INIT_PERIPH_CLOCK(RCC_USART234578CLKSOURCE_D2PCLK1);
        __HAL_RCC_USART6_CLK_ENABLE();
    } else {
        return;
    }

    /* GPIO configuration */
    for (int i = 0; i < NUM_GPIO_PINS_PER_UART; i++) {
        if (cfg->gpio.by_index[i].port && cfg->gpio.by_index[i].pins) {
            enableGPIOClocksForPort(cfg->gpio.by_index[i].port);
            GPIO_InitStruct.Pin = cfg->gpio.by_index[i].pins;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = i == 1 ? GPIO_PULLUP : GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            GPIO_InitStruct.Alternate = gpioFunc;
            HAL_GPIO_Init(cfg->gpio.by_index[i].port, &GPIO_InitStruct);
        }
    }

    /* Ring buffer initialization */
    if (
        lwrb_init(&state->rx_buffer, uart_rx_buffers[state - uart_phy_state], DMA_BUFFER_SIZE) == 0) {
        /* 0 means failure in lwrb_init() */
        return;
    }
    if (lwrb_init(&state->tx_buffer, uart_tx_buffers[state - uart_phy_state], DMA_BUFFER_SIZE) == 0) {
        /* 0 means failure in lwrb_init() */
        lwrb_free(&state->rx_buffer);
        return;
    }

    /* IRQ configuration */
    if (cfg->irq) {
        /* Priority 5 is the highest (i.e. smallest numeric value) that is
         * allowed without interfering with FreeRTOS */
        HAL_NVIC_SetPriority(cfg->irq, 5, 0);
        HAL_NVIC_EnableIRQ(cfg->irq);
    }

    /* DMA configuration for RX */
    if (cfg->dma_rx) {
        state->dma_rx_handle.Instance = cfg->dma_rx;

#ifdef STM32H7
        if (huart->Instance == USART1) {
            state->dma_rx_handle.Init.Request = DMA_REQUEST_USART1_RX;
        } else if (huart->Instance == USART2) {
            state->dma_rx_handle.Init.Request = DMA_REQUEST_USART2_RX;
        } else if (huart->Instance == USART3) {
            state->dma_rx_handle.Init.Request = DMA_REQUEST_USART3_RX;
        } else {
            return;
        }
#else
        if (huart->Instance == USART1) {
            state->dma_rx_handle.Init.Channel = DMA_CHANNEL_0;
        } else if (huart->Instance == USART2) {
            state->dma_rx_handle.Init.Channel = DMA_CHANNEL_1;
        } else if (huart->Instance == USART3) {
            state->dma_rx_handle.Init.Channel = DMA_CHANNEL_2;
        } else {
            return;
        }
#endif

        state->dma_rx_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
        state->dma_rx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
        state->dma_rx_handle.Init.MemInc = DMA_MINC_ENABLE;
        state->dma_rx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        state->dma_rx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        state->dma_rx_handle.Init.Mode = DMA_CIRCULAR;
        state->dma_rx_handle.Init.Priority = DMA_PRIORITY_LOW;
        state->dma_rx_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&state->dma_rx_handle) != HAL_OK) {
            state->dma_rx_handle.Instance = nullptr; /* to prevent HAL_DMA_Deinit */
            return;
        }

        __HAL_LINKDMA(huart, hdmarx, state->dma_rx_handle);
    } else {
        state->dma_rx_handle.Instance = nullptr;
    }

    /* DMA configuration for TX */
    if (cfg->dma_tx) {
        state->dma_tx_handle.Instance = cfg->dma_tx;

#ifdef STM32H7
        if (huart->Instance == USART1) {
            state->dma_tx_handle.Init.Request = DMA_REQUEST_USART1_TX;
        } else if (huart->Instance == USART2) {
            state->dma_tx_handle.Init.Request = DMA_REQUEST_USART2_TX;
        } else if (huart->Instance == USART3) {
            state->dma_tx_handle.Init.Request = DMA_REQUEST_USART3_TX;
        } else {
            return;
        }
#else
        if (huart->Instance == USART1) {
            state->dma_tx_handle.Init.Channel = DMA_CHANNEL_0;
        } else if (huart->Instance == USART2) {
            state->dma_tx_handle.Init.Channel = DMA_CHANNEL_1;
        } else if (huart->Instance == USART3) {
            state->dma_tx_handle.Init.Channel = DMA_CHANNEL_2;
        } else {
            return;
        }
#endif

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

    if (state->dma_rx_handle.Instance) {
        teller::hal::dma::assignHandle(&state->dma_rx_handle);
    };

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
    } else if (huart->Instance == UART4) {
        __HAL_RCC_UART4_CLK_DISABLE();
    } else if (huart->Instance == UART5) {
        __HAL_RCC_UART5_CLK_DISABLE();
    } else if (huart->Instance == USART6) {
        __HAL_RCC_USART6_CLK_DISABLE();
    }

    for (int i = NUM_GPIO_PINS_PER_UART - 1; i >= 0; i--) {
        if (cfg->gpio.by_index[i].port && cfg->gpio.by_index[i].pins) {
            HAL_GPIO_DeInit(cfg->gpio.by_index[i].port, cfg->gpio.by_index[i].pins);
        }
    }

    if (cfg->irq) {
        HAL_NVIC_DisableIRQ(cfg->irq);
    }

    if (cfg->dma_rx && state->dma_rx_handle.Instance) {
        HAL_DMA_DeInit(&state->dma_rx_handle);
    }

    if (cfg->dma_tx && state->dma_tx_handle.Instance) {
        HAL_DMA_DeInit(&state->dma_tx_handle);
    }

    if (state) {
        osEventFlagsDelete(state->event);
        state->initialized = false;
    }

    detachHandle(huart);

    if (state->dma_rx_handle.Instance) {
        teller::hal::dma::detachHandle(&state->dma_rx_handle);
    };

    if (state->dma_tx_handle.Instance) {
        teller::hal::dma::detachHandle(&state->dma_tx_handle);
    };

    lwrb_free(&state->rx_buffer);
    lwrb_free(&state->tx_buffer);
}
}

/* ************************************************************************** */

/* IRQ handlers */

extern "C" {

void USART1_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[1];
    if (ptr) {
        uart_irq_handler(ptr);
    }
}

void USART2_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[2];
    if (ptr) {
        uart_irq_handler(ptr);
    }
}

void USART3_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[3];
    if (ptr) {
        uart_irq_handler(ptr);
    }
}

void UART4_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[4];
    if (ptr) {
        uart_irq_handler(ptr);
    }
}

void UART5_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[5];
    if (ptr) {
        uart_irq_handler(ptr);
    }
}

void USART6_IRQHandler(void)
{
    UART_HandleTypeDef* ptr = uart_handle_ptrs[6];
    if (ptr) {
        uart_irq_handler(ptr);
    }
}
}

/* ************************************************************************** */

void uart_irq_handler(UART_HandleTypeDef* ptr)
{
    uart_phy_state_t* state = find_uart_phy_state(ptr);
    if (state == nullptr) {
        return;
    }

#ifdef STM32H7
#define ISR_REG uart->ISR
#define RDR_REG uart->TDR
#define TDR_REG uart->TDR
#define ORE_FLAG USART_ISR_ORE
#define TXE_FLAG USART_ISR_TXE_TXFNF
#define RXNE_FLAG USART_ISR_RXNE_RXFNE
#else
#define ISR_REG uart->SR
#define RDR_REG uart->DR
#define TDR_REG uart->DR
#define ORE_FLAG USART_SR_ORE
#define TXE_FLAG USART_SR_TXE
#define RXNE_FLAG USART_SR_RXNE
#endif

    USART_TypeDef* uart = state->handle.Instance;
    uint32_t flags = READ_REG(ISR_REG);
    uint8_t ch;

    if (flags & TXE_FLAG) {
        /* TX register empty so we can transmit the next byte from the TX buffer */
        if (lwrb_read(&state->tx_buffer, &ch, 1)) {
            TDR_REG = (uint16_t)ch;
        } else {
            /* No more bytes, clear the TXEIE interrupt and send an event to
             * the task that was blocked on the write */
            CLEAR_BIT(state->handle.Instance->CR1, USART_CR1_TXEIE);
            osEventFlagsSet(state->event, EVT_WRITTEN);
        }
    }

    if (flags & RXNE_FLAG) {
        /* RX register not empty, write the received byte into the RX buffer */
        ch = RDR_REG & 0x00FFU;
        if (lwrb_write(&state->rx_buffer, &ch, 1) == 0) {
            /* dropped */
            ch = 0;
        }
        osEventFlagsSet(state->event, EVT_READ);
    }

    if (flags & USART_SR_ORE) {
        /* Overrun error, just clear the flag */
#ifdef STM32H7
        uart->ICR = UART_CLEAR_OREF;
#endif
        /* TODO: implement for STM32F4 */
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
