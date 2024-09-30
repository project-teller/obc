#include "config.h"

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "storage/stm32.cpp"
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board
#include "storage/stm32.cpp"
#elif defined(TELLER_BOARD_POSIX)
// Simulator for a POSIX-compliant OS
#include "storage/posix.cpp"
#else
// No storage subsystem on this board
#include "storage/posix.cpp"
#endif
