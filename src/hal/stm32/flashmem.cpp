#include "config.h"

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "flashmem_w25qxx.cpp"
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board
#include "flashmem_w25qxx.cpp"
#else
// No flash memory on this board
#include "flashmem_dummy.cpp"
#endif
