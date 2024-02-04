#pragma once

#include "core/telem/heartbeat.h"
#include "core/telem/timesync.h"

namespace teller::telem {

/**
 * @brief Updates a heartbeat data object from the current system status.
 */
void updateHeartbeatData(frames::heartbeat_data_t* data);

/**
 * @brief Updates a timesync data object from the current system and RTC time.
 */
void updateTimesyncData(frames::timesync_data_t* data);

}
