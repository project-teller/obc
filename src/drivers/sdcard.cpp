#include "config.h"

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "sdcard/spi.cpp"
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board
#include "sdcard/spi.cpp"
#elif defined(TELLER_BOARD_POSIX)
// Simulator for a POSIX-compliant OS
#include "sdcard/posix.cpp"
#else
// No SD card on this board
#include "sdcard/dummy.cpp"
#endif
