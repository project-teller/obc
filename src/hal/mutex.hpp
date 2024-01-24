#pragma once

#include "config.h"

#ifdef TELLER_BOARD_POSIX
#include "hal/posix/mutex.hpp"
#else
#include "hal/stm32/mutex.hpp"
#endif

namespace teller::hal::system {

}
