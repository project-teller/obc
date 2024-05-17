#pragma once

#include "config.h"

#ifdef TELLER_BOARD_POSIX
#include "hal/posix/event_flags.hpp"
#else
#include "hal/stm32/event_flags.hpp"
#endif
