#pragma once

#include "core/telem.h"

namespace teller::telem {

/**
 * @brief Updates a heartbeat data object from the current system status.
 */
void updateHeartbeatData(frames::heartbeat_data_t* data);

}
