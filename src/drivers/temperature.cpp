#include "config.h"

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "temperature/dummy.cpp"
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board v2.1
#include "temperature/adt7301.cpp"
#else
// No temperature sensor on this board
#include "temperature/dummy.cpp"
#endif
