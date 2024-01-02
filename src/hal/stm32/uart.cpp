#include "hal/uart.h"
#include <cassert>
#include <cstring>
#include <stdexcept>

#include "stm32_hal.h"
#include <cmsis_os2.h>

using namespace std;
using namespace teller::hal::uart;

typedef struct {
    USART_TypeDef* instance;
    uint32_t baud_rate;
    bool hw_flow_control;
} uart_config_t;

typedef struct {
    UART_HandleTypeDef handle;
    osEventFlagsId_t event;
} uart_state_t;

static const uint32_t EVT_READ = 0x00000001U;
static const uint32_t EVT_WRITTEN = 0x00000002U;

/* NOTE: Each UART must be mapped to a different UART_HandleTypeDef. This is
 * not checked explicitly. */

#if defined STM32F4
// STM32F4-Discovery
const uart_config_t uart_config[NUM_UARTS] = {
    { USART1 }, /* Telemetry UART */
    { USART2 }, /* Debug UART */
};
#else
// No UART supported on this hardware
const uart_config_t uart_config[NUM_UARTS] = {
    { 0 }, /* Telemetry UART */
    { 0 }, /* Debug UART */
};
#endif

static bool configure_uart(uart_state_t* state, const uart_config_t* cfg);

static uart_state_t uart_state[NUM_UARTS];

bool teller::hal::uart::init()
{
    for (size_t i = 0; i < NUM_UARTS; i++) {
        if (!configure_uart(&uart_state[i], &uart_config[i])) {
            return false;
        }
    }

    return true;
}

bool teller::hal::uart::write(uart_t index, uint8_t* data, uint16_t size)
{
    assert(index >= 0 && index < NUM_UARTS);

    uart_state_t* pState = &uart_state[index];
    UART_HandleTypeDef* pHandle = &pState->handle;

    if (HAL_UART_Transmit_IT(pHandle, data, size) != HAL_OK) {
        return false;
    }

    osEventFlagsWait(pState->event, EVT_WRITTEN, osFlagsWaitAny, osWaitForever);

    return true;
}

bool teller::hal::uart::write(uart_t index, const char* data)
{
    return write(index, reinterpret_cast<uint8_t*>(const_cast<char*>(data)), strlen(data));
}

bool teller::hal::uart::write(uart_t index, const std::string& data)
{
    return write(index, data.c_str());
}

/* ************************************************************************** */

static bool configure_uart(uart_state_t* state, const uart_config_t* cfg)
{
    if (cfg->instance == nullptr) {
        return true;
    }

    UART_HandleTypeDef* pHandle = &state->handle;

    pHandle->Instance = cfg->instance;
    pHandle->Init.BaudRate = cfg->baud_rate > 0 ? cfg->baud_rate : 57600;
    pHandle->Init.WordLength = UART_WORDLENGTH_8B;
    pHandle->Init.StopBits = UART_STOPBITS_1;
    pHandle->Init.Parity = UART_PARITY_NONE;
    pHandle->Init.Mode = UART_MODE_TX_RX;
    pHandle->Init.HwFlowCtl = cfg->hw_flow_control ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
    pHandle->Init.OverSampling = UART_OVERSAMPLING_16;

#ifdef USART1
    if (cfg->instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
    }
#endif

#ifdef USART2
    if (cfg->instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
    }
#endif

#ifdef USART3
    if (cfg->instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
    }
#endif

    if (HAL_UART_Init(pHandle) != HAL_OK) {
        return false;
    }

    state->event = osEventFlagsNew(NULL);
    if (!state->event) {
        HAL_UART_DeInit(pHandle);
        return false;
    }

    /* TODO: we will probably have to enable the TX/RX IRQ but I don't know
     * how to do it yet */

    return true;
}

/* TODO: redirect printf to debug UART by overriding int __io_putchar(int ch) */

/* ************************************************************************** */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* uart)
{
    for (size_t i = 0; i < NUM_UARTS; i++) {
        if (uart == &uart_state[i].handle) {
            osEventFlagsSet(uart_state[i].event, EVT_WRITTEN);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* uart)
{
    for (size_t i = 0; i < NUM_UARTS; i++) {
        if (uart == &uart_state[i].handle) {
            osEventFlagsSet(uart_state[i].event, EVT_READ);
        }
    }
}

int __io_putchar(int ch)
{
    uint8_t to_write = ch;
    write(DEBUG, &to_write, 1);
    return ch;
}
