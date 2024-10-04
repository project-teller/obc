#include "config.h"

#if defined(TELLER_BOARD_NUCLEO144)
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "mag/dummy.cpp"
#elif defined(TELLER_BOARD_STM32F4)
// STM32F415RG TELLER OBC board
#include "mag/mlx90393.cpp"
#elif defined(TELLER_BOARD_POSIX)
// Simulator for a POSIX-compliant OS
#include "mag/posix.cpp"
#else
// No IMU on this board
#include "mag/dummy.cpp"
#endif
