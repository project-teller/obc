#include "config.h"

#if defined TELLER_BOARD_NUCLEO144
// STM32H743ZI Nucleo-144 dev board, for testing purposes
#include "imu_icm_20649.cpp"
#else
// No IMU on this board
#include "imu_dummy.cpp"
#endif
