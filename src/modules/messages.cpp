#include "hal/system.h"

#include "modules/errors.h"
#include "modules/messages.h"

using namespace teller::errors;
using namespace teller::hal::system;
using namespace teller::telem;

void teller::telem::updateHeartbeatData(frames::heartbeat_data_t* data)
{
    data->timestampInMsec = getTimeSinceBootMsec();
    data->error = getError();
}
