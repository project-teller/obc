#include "config.h"

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "sdcard_spi.cpp"
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC
#include "sdcard_dummy.cpp"
#else
// No SD card on this board
#include "sdcard_dummy.cpp"
#endif
