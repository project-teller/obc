#include "config.h"

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "adc/dummy.cpp"
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board
#include "adc/dummy.cpp"
#elif defined(TELLER_BOARD_POSIX)
// Simulator for a POSIX-compliant OS
#include "adc/dummy.cpp"
#else
// No ADC on this board
#include "adc/dummy.cpp"
#endif
