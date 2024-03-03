#pragma once

#include "config.h"

#ifdef TELLER_BOARD_POSIX
#include "hal/posix/queue.hpp"
#else
#include "hal/stm32/queue.hpp"
#endif
