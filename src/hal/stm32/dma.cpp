#include "config.h"
#include "stm32_hal.h"

#include "hal/dma.h"
#include "hal/led.h"

typedef struct {
    DMA_TypeDef* dma;
    IRQn_Type irq;
} dma_config_t;

#define NO_MORE_DMA_CHANNELS \
    {                        \
        0                    \
    }

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
const dma_config_t dma_config[] = {
    NO_MORE_DMA_CHANNELS
};
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board
const dma_config_t dma_config[] = {
    { DMA1, DMA1_Stream3_IRQn }, // for SPI2 RX
    { DMA1, DMA1_Stream4_IRQn }, // for SPI2 TX
    { DMA2, DMA2_Stream2_IRQn }, // for SPI1 RX
    { DMA2, DMA2_Stream3_IRQn }, // for SPI1 TX
    NO_MORE_DMA_CHANNELS
};
#else
// No DMA channels needed
const dma_config_t dma_config[] = {
    NO_MORE_DMA_CHANNELS
};
#endif

static DMA_HandleTypeDef* dma1_handle_ptrs[5];
static DMA_HandleTypeDef* dma2_handle_ptrs[5];

namespace teller::hal::dma {

bool init()
{
    const dma_config_t* cfg;

    for (cfg = dma_config; cfg->dma; cfg++) {
        if (cfg->dma == DMA1) {
            __HAL_RCC_DMA1_CLK_ENABLE();
        } else if (cfg->dma == DMA2) {
            __HAL_RCC_DMA2_CLK_ENABLE();
        }

        HAL_NVIC_SetPriority(cfg->irq, 5, 0);
        HAL_NVIC_EnableIRQ(cfg->irq);
    }

    return true;
}

void destroy()
{
    const dma_config_t* cfg;

    for (cfg = dma_config; cfg->dma; cfg++) {
        HAL_NVIC_DisableIRQ(cfg->irq);

        if (cfg->dma == DMA1) {
            __HAL_RCC_DMA1_CLK_DISABLE();
        } else if (cfg->dma == DMA2) {
            __HAL_RCC_DMA2_CLK_DISABLE();
        }
    }
}

void assignHandle(DMA_HandleTypeDef* handle)
{
    if (handle->Instance == DMA1_Stream0) {
        dma1_handle_ptrs[0] = handle;
    } else if (handle->Instance == DMA1_Stream1) {
        dma1_handle_ptrs[1] = handle;
    } else if (handle->Instance == DMA1_Stream2) {
        dma1_handle_ptrs[2] = handle;
    } else if (handle->Instance == DMA1_Stream3) {
        dma1_handle_ptrs[3] = handle;
    } else if (handle->Instance == DMA1_Stream4) {
        dma1_handle_ptrs[4] = handle;
    } else if (handle->Instance == DMA2_Stream0) {
        dma2_handle_ptrs[0] = handle;
    } else if (handle->Instance == DMA2_Stream1) {
        dma2_handle_ptrs[1] = handle;
    } else if (handle->Instance == DMA2_Stream2) {
        dma2_handle_ptrs[2] = handle;
    } else if (handle->Instance == DMA2_Stream3) {
        dma2_handle_ptrs[3] = handle;
    } else if (handle->Instance == DMA2_Stream4) {
        dma2_handle_ptrs[4] = handle;
    }

    /* TODO(ntamas): add more if we start using more DMA channels */
}

void detachHandle(DMA_HandleTypeDef* handle)
{
    int i, n;

    n = sizeof(dma1_handle_ptrs) / sizeof(dma1_handle_ptrs[0]);
    for (i = 0; i < n; i++) {
        if (handle->Instance == dma1_handle_ptrs[i]->Instance) {
            dma1_handle_ptrs[i] = nullptr;
        }
    }

    n = sizeof(dma2_handle_ptrs) / sizeof(dma2_handle_ptrs[0]);
    for (i = 0; i < n; i++) {
        if (handle->Instance == dma2_handle_ptrs[i]->Instance) {
            dma2_handle_ptrs[i] = nullptr;
        }
    }
}

}

/* ************************************************************************** */

/* IRQ handlers */

extern "C" {

void DMA1_Stream0_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma1_handle_ptrs[0];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA1_Stream1_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma1_handle_ptrs[1];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA1_Stream2_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma1_handle_ptrs[2];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA1_Stream3_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma1_handle_ptrs[3];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA1_Stream4_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma1_handle_ptrs[4];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA2_Stream0_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma2_handle_ptrs[0];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA2_Stream1_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma2_handle_ptrs[1];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA2_Stream2_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma2_handle_ptrs[2];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA2_Stream3_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma2_handle_ptrs[3];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}

void DMA2_Stream4_IRQHandler(void)
{
    DMA_HandleTypeDef* ptr = dma2_handle_ptrs[4];
    if (ptr) {
        HAL_DMA_IRQHandler(ptr);
    }
}
}
