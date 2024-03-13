#pragma once

#include "core/telem/clock_status.h"
#include "core/telem/heartbeat.h"

namespace teller::telem {

/**
 * @brief Updates a heartbeat data object from the current system status.
 */
void updateHeartbeatData(frames::heartbeat_data_t* data);

/**
 * @brief Updates a clock status data object from the current system and RTC time.
 */
void updateClockStatusData(frames::clock_status_data_t* data);

}
