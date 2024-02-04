#include "hal/rtc.h"
#include "hal/system.h"

#include "modules/errors.h"
#include "modules/messages.h"

using namespace teller::errors;
using namespace teller::hal;
using namespace teller::telem;

void teller::telem::updateHeartbeatData(frames::heartbeat_data_t* data)
{
    data->timestampInMsec = system::getTimeSinceBootMsec();
    data->error = getError();
}

void teller::telem::updateTimesyncData(frames::timesync_data_t* data)
{
    data->timestampInMsec = system::getTimeSinceBootMsec();
    data->rtcTimestampInMsec = rtc::getTimeMsec();
}
