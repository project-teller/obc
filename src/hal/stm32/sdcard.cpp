#include "config.h"

#if defined TELLER_BOARD_NUCLEO144
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "sdcard_spi.cpp"
#else
// No flash memory on this board
#include "sdcard_dummy.cpp"
#endif
